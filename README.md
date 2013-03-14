php5-proj
=========

A php extension for proj.4

Code example (see: [ProjAPI](http://trac.osgeo.org/proj/wiki/ProjAPI)):
-------------
	<?php  
	$pj_merc = pj_init_plus("+proj=merc +ellps=clrk66 +lat_ts=33");  
	$pj_latlong = pj_init_plus("+proj=latlong +ellps=clrk66");  
	if ($pj_merc !== false && $pj_latlong !== false) {  
		$x = deg2rad(-16);  
		$y = deg2rad(20.25);  
		$transformed = pj_transform($pj_merc, $pj_latlong, 1, 1, $x, $y);  
		print 'latitude: '.$transformed['x'].'<br />';  
		print 'longitude: '.$transformed['y'].'<br />';  
	}  
	?>

Output:
-------
latitude: -1495284.2114735  
longitude: 1920596.7899174
