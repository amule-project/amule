<!doctype html>
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
<body class="frame">
<table class="w100p h30">
  <tr class="va-top"> 
    <td class="w65p"><strong>Ed2k : </strong> 
      <?php
    	$stats = amule_get_stats();
    	if ( $stats["id"] == 0 ) {
    		echo "Not connected";
    	} elseif ( $stats["id"] == 0xffffffff ) {
    		echo "Connecting ...";
    	} else {
    		echo "Connected with ", (($stats["id"] < 16777216) ? "low" : "high"), " ID to ",
    			htmlspecialchars($stats["serv_name"]), "  ", htmlspecialchars($stats["serv_addr"]);
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
