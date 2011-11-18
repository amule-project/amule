<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule control page</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<meta http-equiv="pragmas" content="no-cache">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}
?>
<style type="text/css">
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
</html>
