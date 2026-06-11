<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<meta http-equiv="pragmas" content="no-cache">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}
?>
<link href="style.css" rel="stylesheet" type="text/css">
</head>
<body>
<table width="100%" height="30" border="0" align="center" cellpadding="0" cellspacing="0">
  <tr valign="top"> 
    <td width="65%"><strong>Ed2k : </strong> 
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
    <td><strong>Kad :</strong> 
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
  </tr>
</table>
</body>
</html>
