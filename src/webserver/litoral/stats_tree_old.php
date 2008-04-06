<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule preferences page</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
<meta http-equiv="pragmas" content="no-cache">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}
?>
<link href="style.css" rel="stylesheet" type="text/css"><style type="text/css">
<!--
caption {
	font-family: Helvetica;
	font-size: 18px;
	font-weight: bold;
	color: #003161;
}
th {
	font-family: Helvetica;
	font-size: 14px;
	font-height: 22px;
	font-weight: bold;
	color: #003161;
}
a:link {
	color: #003161;
	text-decoration: none;
}
a:active {
	color: #003161;
	text-decoration: none;
}
a:visited {
	color: #003161;
	text-decoration: none;
}
a:hover {
	color: #c0c0c0;
	text-decoration: underline;
}
td {
	font-family: Helvetica;
	font-size: 12px;
	font-weight: normal;
}
label {
	font-family: Helvetica;
	font-size: 14px;
	font-weight: bold;
}
.texte {
	font-family: Helvetica;
	font-size: 12px;
	font-weight: normal;
}
label {
font-family:"trebuchet ms",sans-serif;
font-size: 12px;
font-weight:bold
}
input {
border:1px solid #003161;
background-color:  white;
font-family:"trebuchet ms",sans-serif;
font-size: 12px;
color: #003161;
}
select, option {
background-color:  white;
font-size: 12px;
color: #003161;
}
textarea {
border:1px solid #003161;
background-color: #90B6DB;
font-family:"trebuchet ms",sans-serif;
font-size: 12px;
color: white;
}
-->
</style>
</head>
<script language="JavaScript" type="text/JavaScript">
var openImg = new Image();
openImg.src = "images/tree-open.gif";
var closedImg = new Image();
closedImg.src = "images/tree-closed.gif";

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
<body onLoad="showBranch('br_Stats');swapFolder('fl_Stats')"><span class="texte">
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
	echo "<img src=\"images/tree-leaf.gif\">", $it, "<br>\n";
}

function print_folder($key, &$arr, $ident)
{
	print_ident($ident);
	echo "<span class=\"texte\" onClick=\"showBranch('br_",
		$key, "');swapFolder('fl_", $key, "')\">\n";
	print_ident($ident+1);
	echo "<img src=\"images/tree-closed.gif\" border=\"0\" id=\"fl_", $key, "\">\n";
	print_ident($ident+1);
	echo $key, "<br>\n";
	print_ident($ident);
	echo "</span>\n";
	print_ident($ident);
	echo "<span class=\"texte\" id=\"br_", $key, "\">\n";

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

	foreach ($stattree as $k => $v) {
		if ( count(&$v) ) {
			print_folder($k, $v, $ident+1);
		} else {
			print_item($k, $ident+1);
		}
	}
?></span>
</body>
</html>
