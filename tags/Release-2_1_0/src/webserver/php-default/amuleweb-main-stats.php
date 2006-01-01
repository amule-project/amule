<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule Kad page</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
<meta http-equiv="pragmas" content="no-cache">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}

	amule_load_vars("stats_graph");

?>
<style type="text/css">
<!--
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background-color: #003399;
	font-size: small;
	font-family: Tahoma;
	color: #FFFFFF;
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
<body>
<table width="904" border="0" cellpadding="0" cellspacing="0">
  <!--DWLayoutTable-->
  <tr>
    <td width="11">&nbsp;</td>
    <td width="379">&nbsp;</td>
    <td width="10" height="10">&nbsp;</td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td rowspan="7"><iframe name="stats" src="stats_tree.php" width="400" height="700" frameborder="1"></iframe></td>
    <td height="58"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td><img src="amule_stats_download.png" width="500" height="200" border="0" alt="" title="" /></td>
  </tr>
  <tr valign="top">
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td height="20"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td><div align="center">Download speed </div></td>
    </tr>
  <tr valign="top">
    <td>&nbsp;</td>
    <td height="35">&nbsp;</td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td height="58"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td><img src="amule_stats_upload.png" width="500" height="200" border="0" alt="" title="" /></td>
  </tr>
  <tr valign="top">
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td height="20"><div align="center"></div></td>
    <td><div align="center">Upload speed</div></td>
    </tr>
  <tr valign="top">
    <td>&nbsp;</td>
    <td height="35">&nbsp;</td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td height="58"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td><img src="amule_stats_conncount.png" width="500" height="200" border="0" alt="" title="" /></td>
  </tr>
  <tr valign="top">
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td height="20"><div align="center"></div></td>
    <td><div align="center">Number of connections </div></td>
    </tr>
  <tr valign="top">
    <td>&nbsp;</td>
    <td>&nbsp;</td>
    <td height="35">&nbsp;</td>
    <td>&nbsp;</td>
  </tr>
</table>
</body>
</html>
