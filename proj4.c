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
	zval *zpj_latlong, *zpj_merc;
	projPJ pj_latlong, pj_merc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrlldd|d", &zpj_latlong, &zpj_merc, &point_count, &point_offset, &x, &y, &z) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(pj_latlong, projPJ*, &zpj_latlong, -1, PHP_PROJ4_RES_NAME, le_proj4);
	ZEND_FETCH_RESOURCE(pj_merc, projPJ*, &zpj_merc, -1, PHP_PROJ4_RES_NAME, le_proj4);

	if (point_count != 1) {
		RETURN_FALSE;
	}
	if (point_offset != 1) {
		RETURN_FALSE;
	}

	p = pj_transform(pj_latlong, pj_merc, point_count, point_offset, &x, &y, &z);
	if (p != 0) {
		RETURN_DOUBLE(p);
	}

	array_init(return_value);
	add_assoc_double(return_value, "x", x);
	add_assoc_double(return_value, "y", y);
	add_assoc_double(return_value, "z", z);
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
