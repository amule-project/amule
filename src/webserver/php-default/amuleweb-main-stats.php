<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule preferences page</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
<style type="text/css">
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

</script>
<?php
	amule_load_vars("stats_graph");
	
	$stattree = amule_load_vars("stats_tree");
	var_dump($stattree);
?>
<body onload="init_data();" >
<form name="mainform" action="amuleweb-main-prefs.php" method="post">
<table width="800" border="0" cellpadding="0" cellspacing="0">
  <!--DWLayoutTable-->
  <tr>
    <td width="52" height="10"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td width="383">&nbsp;</td>
    <td width="365">&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="58">&nbsp;</td>
    <td><img src="amule_stats_download.png" width="286" height="200" border="0" alt="" title="" /></td>
    <td><img src="amule_stats_upload.png" width="286" height="200" border="0" alt="" title="" /></td>
  </tr>
  <tr valign="top">
    <td height="35">&nbsp;</td>
    <td>&nbsp;</td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="62">&nbsp;</td>
    <td><img src="amule_stats_conncount.png" width="286" height="200" border="0" alt="" title="" /></td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="108">&nbsp;</td>
    <td><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td>&nbsp;</td>
  </tr>
</table>
</form>
</body>
</html>
