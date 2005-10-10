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
}
.branch{
	display: none;
	margin-left: 16px;
}
<!--
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background-color: #003399;
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
<?php
	amule_load_vars("stats_graph");
?>
<body>
<form name="mainform" action="amuleweb-main-prefs.php" method="post">
<table width="800" border="0" cellpadding="0" cellspacing="0">
  <!--DWLayoutTable-->
  <tr>
    <td width="52" height="10"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td colspan="2">&nbsp;</td>
    <td colspan="2">&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="58">&nbsp;</td>
    <td colspan="2"><img src="amule_stats_download.png" width="286" height="200" border="0" alt="" title="" /></td>
    <td colspan="2"><img src="amule_stats_upload.png" width="286" height="200" border="0" alt="" title="" /></td>
  </tr>
  <tr valign="top">
    <td height="20">&nbsp;</td>
    <td width="285"><div align="center">Downloads</div></td>
    <td width="98">&nbsp;</td>
    <td width="286"><div align="center">Uploads</div></td>
    <td width="79">&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="35">&nbsp;</td>
    <td colspan="2">&nbsp;</td>
    <td colspan="2">&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="62">&nbsp;</td>
    <td colspan="2"><img src="amule_stats_conncount.png" width="286" height="200" border="0" alt="" title="" /></td>
    <td colspan="2">
		<iframe name="stats" src="stats_tree.php" scrolling="yes" frameborder="1"></iframe>
    </td>
  </tr>
  <tr valign="top">
    <td height="20">&nbsp;</td>
    <td><div align="center">Number of connections </div></td>
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td colspan="2"><!--DWLayoutEmptyCell-->&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="108">&nbsp;</td>
    <td colspan="2"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td colspan="2">&nbsp;</td>
  </tr>
</table>
</form>
</body>
</html>
