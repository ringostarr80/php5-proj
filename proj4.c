#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <php_main.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#include <proj_api.h>
#include "php_proj4.h"

int le_proj4;

static zend_function_entry proj4_functions[] = {
    ZEND_FE(pj_init_plus, NULL)
	ZEND_FE(pj_transform, NULL)
	ZEND_FE(pj_is_latlong, NULL)
	ZEND_FE(pj_is_geocent, NULL)
	ZEND_FE(pj_get_def, NULL)
	ZEND_FE(pj_latlong_from_proj, NULL)
	ZEND_FE(pj_deallocate_grids, NULL)
	ZEND_FE(pj_strerrno, NULL)
	ZEND_FE(pj_get_errno_ref, NULL)
	ZEND_FE(pj_get_release, NULL)
	ZEND_FE(pj_free, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry proj4_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_PROJ4_EXTNAME,
    proj4_functions,
    PHP_MINIT(proj4),
    PHP_MSHUTDOWN(proj4),
    PHP_RINIT(proj4),
    NULL,
	PHP_MINFO(proj4),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_PROJ4_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

static void php_proj4_dtor(zend_rsrc_list_entry *resource TSRMLS_DC)
{
	projPJ *proj = (projPJ*)resource->ptr;
	if (proj != NULL && proj) {
		pj_free(proj);
	}
}

PHP_RINIT_FUNCTION(proj4)
{
	return SUCCESS;
}

PHP_MINIT_FUNCTION(proj4)
{
	le_proj4 = zend_register_list_destructors_ex(php_proj4_dtor, NULL, PHP_PROJ4_RES_NAME, module_number);

	return SUCCESS;
}

PHP_MINFO_FUNCTION(proj4)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "proj.4 module", "enabled");
	php_info_print_table_row(2, "proj.4 version", pj_get_release());
	php_info_print_table_end();
}

PHP_MSHUTDOWN_FUNCTION(proj4)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}

#ifdef COMPILE_DL_PROJ4
ZEND_GET_MODULE(proj4)
#endif

ZEND_FUNCTION(pj_init_plus)
{
	projPJ pj_merc;
	char *definition;
	int definition_length;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &definition, &definition_length) == FAILURE) {
		RETURN_FALSE;
	}

	pj_merc = pj_init_plus(definition);
	if (pj_merc == NULL) {
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, pj_merc, le_proj4);
}

ZEND_FUNCTION(pj_transform)
{
	int p;
	long point_count, point_offset;
	double x, y, z = 0;
	zval *zx, *zy, *zz;
	zval *zpj_latlong, *zpj_merc;
	projPJ pj_latlong, pj_merc;

	MAKE_STD_ZVAL(zx);
	MAKE_STD_ZVAL(zy);
	MAKE_STD_ZVAL(zz);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrllzz|z", &zpj_latlong, &zpj_merc, &point_count, &point_offset, &zx, &zy, &zz) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(pj_latlong, projPJ*, &zpj_latlong, -1, PHP_PROJ4_RES_NAME, le_proj4);
	ZEND_FETCH_RESOURCE(pj_merc, projPJ*, &zpj_merc, -1, PHP_PROJ4_RES_NAME, le_proj4);

	array_init(return_value);

	if (point_offset < 0) {
		point_offset = 0;
	}
	if (point_count < 0) {
		point_count = 0;
	}

	if ((Z_TYPE_P(zx) == IS_DOUBLE || Z_TYPE_P(zx) == IS_LONG || Z_TYPE_P(zx) == IS_STRING) &&
		(Z_TYPE_P(zy) == IS_DOUBLE || Z_TYPE_P(zy) == IS_LONG || Z_TYPE_P(zy) == IS_STRING)) {
		convert_to_double_ex(&zx);
		x = Z_DVAL_P(zx);
		convert_to_double_ex(&zy);
		y = Z_DVAL_P(zy);

		switch(Z_TYPE_P(zz)) {
			case IS_DOUBLE:
				z = Z_DVAL_P(zz);
				break;

			case IS_LONG:
			case IS_STRING:
				convert_to_double_ex(&zz);
				z = Z_DVAL_P(zz);
				break;
		}

		p = pj_transform(pj_latlong, pj_merc, 1, 0, &x, &y, &z);

		add_assoc_double(return_value, "x", x);
		add_assoc_double(return_value, "y", y);
		add_assoc_double(return_value, "z", z);
	} else if (Z_TYPE_P(zx) == IS_ARRAY && Z_TYPE_P(zy) == IS_ARRAY) {
		zval **x_data, **y_data, **z_data;
		HashTable *x_array, *y_array, *z_array;
		HashPosition x_position, y_position, z_position;
		int x_count, y_count, z_count = 0, max_count = -1, current_array_position = 1, added_array_elements = 0;
		x_array = Z_ARRVAL_P(zx);
		y_array = Z_ARRVAL_P(zy);
		x_count = zend_hash_num_elements(x_array);
		y_count = zend_hash_num_elements(y_array);
		if (Z_TYPE_P(zz) == IS_ARRAY) {
			z_array = Z_ARRVAL_P(zz);
			z_count = zend_hash_num_elements(z_array);
		}
		if (x_count > 0 && y_count > 0) {
			max_count = (x_count <= y_count) ? x_count : y_count;
			if (z_count > 0 && z_count < max_count) {
				max_count = z_count;
			}
			if (point_count < max_count) {
				max_count = point_count;
			}

			double x_input_array[max_count], y_input_array[max_count], z_input_array[max_count];
			int current_input_array_index = 0;

			zend_hash_internal_pointer_reset_ex(y_array, &y_position);
			if (z_count > 0) {
				zend_hash_internal_pointer_reset_ex(z_array, &z_position);
			}

			double current_x = 0, current_y = 0, current_z = 0;
			for(zend_hash_internal_pointer_reset_ex(x_array, &x_position), zend_hash_internal_pointer_reset_ex(y_array, &y_position);
				zend_hash_get_current_data_ex(x_array, (void**)&x_data, &x_position) == SUCCESS && zend_hash_get_current_data_ex(y_array, (void**)&y_data, &y_position) == SUCCESS;
				zend_hash_move_forward_ex(x_array, &x_position), zend_hash_move_forward_ex(y_array, &y_position), current_array_position++) {
				if (current_array_position < point_offset + 1) {
					continue;
				}
				if (added_array_elements >= point_count) {
					break;
				}

				if ((Z_TYPE_PP(x_data) == IS_DOUBLE || Z_TYPE_PP(x_data) == IS_LONG || Z_TYPE_PP(x_data) == IS_STRING) &&
					(Z_TYPE_PP(y_data) == IS_DOUBLE || Z_TYPE_PP(y_data) == IS_LONG || Z_TYPE_PP(y_data) == IS_STRING)) {
					if (z_count > 0) {
						if (zend_hash_get_current_data_ex(z_array, (void**)&z_data, &z_position) == SUCCESS) {
							convert_to_double_ex(z_data);
							current_z = Z_DVAL_PP(z_data);

							zend_hash_move_forward_ex(z_array, &z_position);
						} else {
							RETURN_FALSE;
						}
					}

					if (Z_TYPE_PP(x_data) != IS_DOUBLE) {
						convert_to_double_ex(x_data);
					}
					current_x = Z_DVAL_PP(x_data);
					if (Z_TYPE_PP(y_data) != IS_DOUBLE) {
						convert_to_double_ex(y_data);
					}
					current_y = Z_DVAL_PP(y_data);

					x_input_array[current_input_array_index] = current_x;
					y_input_array[current_input_array_index] = current_y;
					z_input_array[current_input_array_index] = current_z;
					current_input_array_index++;
					added_array_elements++;
				} else {
					RETURN_FALSE;
				}
			}

			p = pj_transform(pj_latlong, pj_merc, max_count, 0, x_input_array, y_input_array, z_input_array);
			if (p == 0) {
				zval *x_array_element, *y_array_element, *z_array_element;
				ALLOC_INIT_ZVAL(x_array_element);
				ALLOC_INIT_ZVAL(y_array_element);
				ALLOC_INIT_ZVAL(z_array_element);
				array_init(x_array_element);
				array_init(y_array_element);
				array_init(z_array_element);

				int i;
				for(i = 0; i < max_count; i++) {
					add_next_index_double(x_array_element, x_input_array[i]);
					add_next_index_double(y_array_element, y_input_array[i]);
					add_next_index_double(z_array_element, z_input_array[i]);
				}

				add_assoc_zval(return_value, "x", x_array_element);
				add_assoc_zval(return_value, "y", y_array_element);
				add_assoc_zval(return_value, "z", z_array_element);
			}
		}
	} else {
		RETURN_FALSE;
	}

	if (p != 0) {
		RETURN_DOUBLE(p);
	}
}

ZEND_FUNCTION(pj_is_latlong)
{
	zval *zpj;
	projPJ pj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpj) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(pj, projPJ*, &zpj, -1, PHP_PROJ4_RES_NAME, le_proj4);

	if (pj_is_latlong(pj)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(pj_is_geocent)
{
	zval *zpj;
	projPJ pj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpj) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(pj, projPJ*, &zpj, -1, PHP_PROJ4_RES_NAME, le_proj4);

	if (pj_is_geocent(pj)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(pj_get_def)
{
	long options = 0;
	char *result;
	zval *zpj;
	projPJ pj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|l", &zpj, &options) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(pj, projPJ*, &zpj, -1, PHP_PROJ4_RES_NAME, le_proj4);

	if (pj != NULL && pj) {
		result = pj_get_def(pj, options);

		RETURN_STRING(result, 1);
	}

	RETURN_FALSE;
}

ZEND_FUNCTION(pj_latlong_from_proj)
{
	zval *zpj_in;
	projPJ pj_in, pj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpj_in) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(pj_in, projPJ*, &zpj_in, -1, PHP_PROJ4_RES_NAME, le_proj4);

	if (pj_in != NULL && pj_in) {
		pj = pj_latlong_from_proj(pj_in);

		if (pj == NULL) {
			RETURN_FALSE;
		}

		ZEND_REGISTER_RESOURCE(return_value, pj, le_proj4);
	} else {
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(pj_deallocate_grids)
{
	pj_deallocate_grids();
}

ZEND_FUNCTION(pj_get_errno_ref)
{
	RETURN_LONG(*pj_get_errno_ref());
}

ZEND_FUNCTION(pj_strerrno)
{
	long error_code;
	char *result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &error_code) == FAILURE) {
		RETURN_FALSE;
	}

	result = pj_strerrno(error_code);

	RETURN_STRING(result, 1);
}

ZEND_FUNCTION(pj_get_release)
{
	const char *result;

	result = pj_get_release();

	RETURN_STRING(result, 1);
}

ZEND_FUNCTION(pj_free)
{
	zval *zpj;
	projPJ pj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpj) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(pj, projPJ*, &zpj, -1, PHP_PROJ4_RES_NAME, le_proj4);

	zend_list_delete(Z_LVAL_P(zpj));
}
