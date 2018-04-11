<?php

//print("Start!\n");

$link = mysql_connect("localhost", "rk6stud", "rk6stud")
        or die("Could not connect : " . mysql_error());
//print("Connected successfully\n");

mysql_select_db("femdb") or die("Could not select database");

$query = 'select props, q1.x, q1.y, q2.x, q2.y, q3.x, q3.y from elements el left join nodes q1 on (el.n1 = q1.id) left join nodes q2 on (el.n2 = q2.id) left join nodes q3 on (el.n3 = q3.id)';
$result = mysql_query($query) or die("Query failed : " . mysql_error());
//print("Got a result!\n");

$triangles = array();

while ($line = mysql_fetch_array($result, MYSQL_NUM)) {
	array_push($triangles, $line);
}
//print("pushed to array!\n");

mysql_free_result($result);
mysql_close($link);


$imgX = 800;
$imgY = 600;
$scale = 3;

header("Content-type: image/png");
$img = @imagecreate($imgX, $imgY)
    or die("Cannot Initialize new GD image stream");
    
$background_color = imagecolorallocate($img, 255, 0, 255);

$colorArray = array(
    "steel" => imagecolorallocate($img, 233, 14, 91),
    "default" => imagecolorallocate($img, 100, 255, 255)
);

foreach ($triangles as $triangle) {
	if (!in_array($triangle[0], array_keys($colorArray))) {
		$triangle[0] = "default";
	}
	$points = array_slice($triangle, 1, 6);
	for ($i = 0; $i < count($points); $i++) {
		$points[$i] = ($points[$i] * $scale) + ($i % 2 ? $imgX/2: $imgY/2);
	}
	//print_r($points);
	imagefilledpolygon($img, $points, 3, $colorArray[$triangle[0]]);
}

imagepng($img);
imagedestroy($img);

?>