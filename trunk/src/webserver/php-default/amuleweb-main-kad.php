<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html><head>
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
.kadnewnode {
	background-color: #336600;
}
-->
</style>
</head>

<body>
<form name="mainform" action="amuleweb-main-kad.php" method="post">
  <table width="868" border="0" cellpadding="0" cellspacing="0">
    <!--DWLayoutTable-->
    <tr>
      <td width="29" height="16"></td>
      <td width="500"></td>
      <td colspan="2"></td>
    </tr>
    <tr valign="top">
      <td height="20"><!--DWLayoutEmptyCell-->&nbsp;</td>
      <td><!--DWLayoutEmptyCell-->&nbsp;</td>
      <td colspan="2"><div align="center"></div></td>
    </tr>
    <tr valign="top">
      <td height="35">&nbsp;</td>
      <td>&nbsp;</td>
      <td colspan="2">&nbsp;</td>
    </tr>
    <tr valign="top">
      <td height="200"><!--DWLayoutEmptyCell-->&nbsp;</td>
      <td><img src="amule_stats_kad.png" width="500" height="200" border="0" alt="" title="" /></td>
      <td width="18"><!--DWLayoutEmptyCell-->&nbsp;</td>
      <td width="321"><table width="100%"  border="0" cellpadding="0" cellspacing="0" class="kadnewnode">
        <tr>
          <th colspan="8" scope="col">Bootstrap from node </th>
          </tr>
        <tr>
          <td width="7%">&nbsp;</td>
          <td width="11%">IP</td>
          <td width="8%">&nbsp;</td>
          <td width="12%"><input name="ip3" type="text" id="ip3" size="4" maxlength="3"></td>
          <td width="12%"><input name="ip2" type="text" id="ip2" size="4" maxlength="3"></td>
          <td width="12%"><input name="ip1" type="text" id="ip1" size="4" maxlength="3"></td>
          <td width="12%"><input name="ip0" type="text" id="ip0" size="4" maxlength="3"></td>
          <td width="26%">&nbsp;</td>
        </tr>
        <tr>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td colspan="3">&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
        </tr>
        <tr>
          <td>&nbsp;</td>
          <td>Port</td>
          <td>&nbsp;</td>
          <td><input name="port" type="text" id="port" size="4" maxlength="5"></td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
        </tr>
        <tr>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
        </tr>
        <tr>
          <td>&nbsp;</td>
          <td colspan="2"><input type="submit" name="Submit" value="Connect"></td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
        </tr>
        <tr>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
        </tr>
      </table></td>
    </tr>
    <tr valign="top">
      <td height="20"><!--DWLayoutEmptyCell-->&nbsp;</td>
      <td><div align="center">Number of nodes </div></td>
      <td colspan="2"><div align="center"></div></td>
    </tr>
    <tr valign="top">
      <td height="35">&nbsp;</td>
      <td>&nbsp;</td>
      <td colspan="2">&nbsp;</td>
    </tr>
    <tr valign="top">
      <td height="24"><!--DWLayoutEmptyCell-->&nbsp;</td>
      <td><!--DWLayoutEmptyCell-->&nbsp;</td>
      <td colspan="2"><!--DWLayoutEmptyCell-->&nbsp;</td>
    </tr>
    <tr valign="top">
      <td height="35">&nbsp;</td>
      <td>&nbsp;</td>
      <td colspan="2"><!--DWLayoutEmptyCell-->&nbsp;</td>
    </tr>
  </table>
</form>
</body>
</html>
