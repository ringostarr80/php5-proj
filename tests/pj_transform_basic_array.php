<?php
$pj_krovak = pj_init_plus('+proj=krovak +lat_0=49.5 +lon_0=24.83333333333333 +alpha=0 +k=0.9999 +x_0=0 +y_0=0 +ellps=bessel');
if ($pj_krovak === false) {
	die(pj_strerrno(pj_get_errno_ref()));
}
$pj_latlong = pj_init_plus("+proj=latlong +towgs84=570.8,85.7,462.8,4.998,1.587,5.261,3.56 +units=m");
if ($pj_latlong === false) {
	die(pj_strerrno(pj_get_errno_ref()));
}
$x = array(-739443);
$y = array(-1045546);
$p = pj_transform($pj_krovak, $pj_latlong, 1, 0, $x, $y);
var_dump($p);
?>
