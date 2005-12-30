<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule control page</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<meta http-equiv="pragmas" content="no-cache">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}
?>
<style type="text/css">
<!--
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background-color: #6699CC;
}
-->
</style>
</head>
<table width="800" border="0" cellpadding="0" cellspacing="0">
  <!--DWLayoutTable-->
  <tr>
    <td width="21" height="9"></td>
    <td width="22"></td>
    <td width="152"></td>
    <td width="16"></td>
    <td width="530"></td>
    <td width="59"></td>
  </tr>
  <tr>
    <td height="20">&nbsp;</td>
    <td valign="top"><img src="connect.gif" width="16" height="16"></td>
    <td valign="top"><strong>Connection status : </strong></td>
    <td>&nbsp;</td>
    <td valign="top">
      <?php
    	$stats = amule_get_stats();
    	if ( $stats["id"] == 0 ) {
    		echo "Not connected";
    	} elseif ( $stats["id"] == 0xffffffff ) {
    		echo "Connecting ...";
    	} else {
    		echo "Connected with ", (($stats["id"] < 16777216) ? "low" : "high"), " ID to ",
    			$stats["serv_name"], "  ", $stats["serv_addr"];
    	}
    ?>
    </td>
    <td>&nbsp;</td>
  </tr>
  <tr>
    <td height="20">&nbsp;</td>
    <td valign="top"><!--DWLayoutEmptyCell-->&nbsp;</td>
    <td valign="top"><strong>Kad status : </strong></td>
    <td>&nbsp;</td>
    <td valign="top">
      <?php
    	$stats = amule_get_stats();
    	if ( $stats["kad_connected"] == 1 ) {
    		echo "Connected";
			if ( $stats["kad_firewalled"] == 1 ) {
				echo "(Firewalled)";
			} else {
				echo "(OK)";
			}
    	} else {
    		echo "Disconnected";
    	}
    ?>
    </td>
    <td>&nbsp;</td>
  </tr>
  <tr>
    <td height="15"></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
</table>


</html>