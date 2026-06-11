<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html><head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}

?>
<style type="text/css">
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background-color: #FFFFFF;
	font-size: small;
	font-family: monospace;
	color: #000000;
}
</style>
</head>
<body>
<pre>
<?php
	if ("srv" == $HTTP_GET_VARS['show'] || 1 == $HTTP_GET_VARS['rstsrv']) {
		echo amule_get_serverinfo($HTTP_GET_VARS['rstsrv']);
	 }else {
		echo amule_get_log($HTTP_GET_VARS['rstlog']);
	}
?>
</pre>
</body>
</html>
