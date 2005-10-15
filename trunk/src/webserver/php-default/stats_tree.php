<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule preferences page</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
<style type="text/css">
.trigger{
	cursor: pointer;
	cursor: hand;
	font-family: Tahoma;
	font-size: small;
}
.branch{
	display: block;
	margin-left: 16px;
	font-family: Tahoma;
	font-size: small;
}
<!--
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background-color: #ffffff;
}
-->
</style>
</head>
<script language="JavaScript" type="text/JavaScript">
var openImg = new Image();
openImg.src = "tree-open.gif";
var closedImg = new Image();
closedImg.src = "tree-closed.gif";

function showBranch(branch){
	var objBranch = document.getElementById(branch).style;
	if(objBranch.display=="block")
		objBranch.display="none";
	else
		objBranch.display="block";
}

function swapFolder(img){
	objImg = document.getElementById(img);
	if(objImg.src.indexOf('tree-closed.gif')>-1)
		objImg.src = openImg.src;
	else
		objImg.src = closedImg.src;
}

</script>
<body onLoad="showBranch('br_Stats');swapFolder('fl_Stats')">
<?php

function print_ident($i)
{
	while($i != 0) {
		echo "\t";
		$i--;
	}
}

function print_item($it, $ident)
{
	print_ident($ident);
	echo "<img src=\"tree-leaf.gif\">", $it, "<br>\n";
}

function print_folder($key, &$arr, $ident)
{
	print_ident($ident);
	echo "<span class=\"trigger\" onClick=\"showBranch('br_",
		$key, "');swapFolder('fl_", $key, "')\">\n";
	print_ident($ident+1);
	echo "<img src=\"tree-closed.gif\" border=\"0\" id=\"fl_", $key, "\">\n";
	print_ident($ident+1);
	echo $key, "<br>\n";
	print_ident($ident);
	echo "</span>\n";
	print_ident($ident);
	echo "<span class=\"branch\" id=\"br_", $key, "\">\n";

	foreach ($arr as $k => $v) {
		if ( count(&$v) ) {
			print_folder($k, $v, $ident+1);
		} else {
			print_item($k, $ident+1);
		}
	}

	print_ident($ident);
	echo "</span>\n";
}

	$stattree = amule_load_vars("stats_tree");
	//var_dump($stattree);
	print_folder("Stats", $stattree["Statistics"], 1);
?>
</body>
</html>
