<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
<META HTTP-EQUIV="PRAGMAS" CONTENT="NO-CACHE">
<title>aMule CVS - Web Control Panel</title>
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"],
			'; url=servers.php', '">';
	}
?>
<style type="text/css">
img
{
border : 0px;
}
a, a:active, a:link, a:visited
{
color: white;
}
.down-header, .down-header-left, .down-header-right,
.down-line, .down-line-good, .down-line-left, .down-line-good-left,
.down-line-right, .down-line-good-right,
.up-header, .up-header-left, .up-line, .up-line-left,
.server-header, .server-header-left, .server-line, .server-line-left,
.shared-header, .shared-header-left, .shared-line, .shared-line-changed, 
.shared-line-left, .shared-line-left-changed,
.header, .smallheader, .commontext,
.upqueue-header, .upqueue-line, .upqueue-line-left,
.websearch-header, .websearch-line, .addserver-header, .addserver-line
{
font-family : Tahoma;
font-size : 8pt;
}
.tabs
{
font-family : Tahoma;
font-size : 10pt;
background-color : #3399FF;
}
.down-header, .down-line, .down-line-good, .up-header, .up-line,
.server-header, .server-line, .shared-header, .shared-line, .shared-line-changed,
.upqueue-header, .upqueue-line,
.websearch-header, .websearch-line, .addserver-header, .addserver-line
{
text-align : center;
}
.down-header-left, .down-line-left, .down-line-good-left,
.server-header-left, .server-line-left, .shared-header-left,
.up-header-left, .up-line-left, .shared-line-left, .shared-line-left-changed, .upqueue-line-left
{
text-align : left;
}
.down-line-right, .down-line-good-right, .down-header-right
{
text-align : right;
}
.down-header, .down-header-left, .down-header-right,
.up-header, .up-header-left, .server-header, .server-header-left,
.shared-header, .shared-header-left, .upqueue-header,
.websearch-header, .addserver-header
{
background-color : #0066CC;
}
.header
{
background-color : #0046AC;
}
.smallheader
{
background-color : #003399;
color : #FFFFFF;
}
.commontext
{
background-color : #FFFFFF;
color : #000000;
}
.down-line, .down-line-good, .down-line-left, .down-line-good-left,
.down-line-right, .down-line-good-right,
.up-line, .up-line-left, .server-line, .server-line-left,
.shared-line, .shared-line-changed, .shared-line-left, .shared-line-left-changed,
.upqueue-line, .upqueue-line-left,
.websearch-line, .addserver-line
{
background-color : #3399FF;
}
.down-line-good, .down-line-good-left, .down-line-good-right, .shared-line-changed, .shared-line-left-changed
{
color : #F0F000;
}
.percent_table
{
border:0px solid #000000;
border-collapse: collapse;
}
.message
{
font-size: 10pt;
font-weight: bold;
color: #FF0000;
}
.dinput
{
border-width: 1px;
border-color: black;
}
</style>
</head>
<body bgcolor="#FBDE9C" text=white link="#3399FF" vlink="#3399FF" alink="#3399FF" marginwidth=0 marginheight=0 topmargin=0 leftmargin=0 style="margin:0px">
<table border=0 width="100%" align=center cellpadding=4 cellspacing=0>
<tr>
 <td class="tabs" align="left" colspan="2">

  <table border="0" cellpadding="4" cellspacing="0">
  <tr>
	<td class="tabs" align="center">
		&nbsp;<a href="http://www.amule.org" target="_blank"><img src="emule.gif"></a>
		<font face="Tahoma" style="font-size:13pt;" color="#000000">aMule<br>Web Control Panel</font>
	</td>
	<td class="tabs" align="center" width="30">&nbsp;		</td>

  	<td align="center" class="tabs" width="95">
  		<a href="servers.php">
  			<img src="cp_servers.gif"><br>
  			Server list
  		</a>
  	</td>
  	<td align="center" class="tabs" width="95">
  		<a href="downloads.php">
  			<img src="cp_download.gif"><br>

  			Transfer
  		</a>
  	</td>
  	<td align="center" class="tabs" width="95">
  		<a href="search.php">
  			<img src="cp_search.gif"><br>
  			Search
  		</a>
  	</td>
  	<td align="center" class="tabs" width="95">

  		<a href="shared.php">
  			<img src="cp_shared.gif"><br>
  			Shared Files
  		</a>
  	<td align="center" class="tabs" width="110">
		<a href="#" onClick="self.location.href='?ses=9024819&amp;w=stats'; return false;">
			<img src="cp_stats.gif"><br>
  			Statistics</a>
  		<font color="#000000">|</font>

  		<a href="#" onClick="self.location.href='?ses=9024819&amp;w=graphs'; return false;">Graphs</a>
  	</td>
  	</td>
  	<td align="center" class="tabs" width="95">
  		<a href="#" onClick="self.location.href='?ses=9024819&amp;w=options'; return false;">
  			<img src="cp_settings.gif"><br>
  			Preferences
  		</a>
  	</td>

  	<td class="tabs" align="center" width="30">&nbsp;</td>
  	<td align="left" class="tabs" width="95">
  		<img src="log.gif"> <a href="#" onClick="self.location.href='?ses=9024819&amp;w=sinfo'; return false;">Serverinfo</a><br>
  		<img src="log.gif"> <a href="#" onClick="self.location.href='?ses=9024819&amp;w=log#end'; return false;">Log</a><!--<br>
  		<img src="log.gif"> <a href="#" onClick="self.location.href='?ses=9024819&amp;w=debuglog#end'; return false;">Debug Log</a>-->
  	</td>
  </tr>
  </table>

 </td>
</tr>
<tr>
<td style="background-color: #000000; height: 1px" colspan="2">
</td>
</tr>
<tr>
 <td class="tabs">
 	&nbsp;&nbsp;<b>Connection:</b>
 	<?php
		function CastToXBytes($size)
		{
			if ( $size < 1024 ) {
				$result = $size . " bytes";
			} elseif ( $size < 1048576 ) {
				$result = ($size / 1024.0) . "KB";
			} elseif ( $size < 1073741824 ) {
				$result = ($size / 1048576.0) . "MB";
			} else {
				$result = ($size / 1073741824.0) . "GB";
			}
			return $result;
		}

		$stats = amule_get_stats();
		if ( $stats["id"] == 0 ) {
			echo "Not connected";
		} elseif ( $stats["id"] == 0xffffffff ) {
			echo "Connecting ...";
		} else {
			echo "Connected with ", (($stats["id"] < 16777216) ? "low" : "high"), " ID to ",
				$stats["serv_name"], "  ", $stats["serv_addr"];
		}
		echo '<br>&nbsp;&nbsp;<b>Speed:</b> Up: ', CastToXBytes($stats["speed_up"]), 'ps',
			' | Down: ', CastToXBytes($stats["speed_down"]), 'ps',
			'<small> (Limits: ', CastToXBytes($stats["speed_limit_up"]), 'ps/',
			CastToXBytes($stats["speed_limit_down"]), 'ps)</small>&nbsp;';
	?>
  <font color=black>

	<script language="javascript">
	var d = new Date();
	s = "[ " + d.getDate() + "/" + (d.getMonth() + 1) + "/" + d.getFullYear() + " " + d.getHours() + ":" + (d.getMinutes() < 10 ? "0" : "") + d.getMinutes() + ":" + (d.getSeconds() < 10 ? "0" : "") + d.getSeconds() + " ]";
	document.write(s);
</script>
  </font>
 </td>
 <td align=right class=tabs>
  <form action="login.html"><input type="button" value="Logout" onClick='self.location.href="login.html"'></form>
 </td>
</tr>
</table>
<font face=Tahoma style="font-size:8pt;">&nbsp;

<table border=0 align=center cellpadding=4 cellspacing=0 width="95%">
<tr>
  <td align=center valign=top>

<?php
	$add_server_form = '
<div class="message"></div>
<table border=0 align=center cellpadding=4 cellspacing=0 width="60%" display="None">
<tr>
<td valign=middle class="addserver-header"><img src="add_server.gif"> <b>Add new server</b></td>
</tr>
<tr>
 <td valign=middle class="addserver-line">
<form action="servers.php" method="GET">
  IP or Address <input name="ip" type="text" size="15">
  Port <input name="port" type="text" size="6">
  Name <input name="name" type="text" size="30"><br>
  <input name="cmd" type="hidden" value="add"><br>
  <input type="submit" value="Add to list">
</form>
 </td>
</tr>
<tr>
<td valign=middle class="addserver-header"><img src="add_server.gif"> <b>Update server.met from URL</b></td>
</tr>
<tr>
 <td valign=middle class="addserver-line">
<form action="servers.php" method="GET">
  URL <input name="servermeturl" type="text" size="60"><br>
  <input type="hidden" name=w value="server">
  <input type="hidden" name=c value="options">
  <input name="updateservermetfromurl" type="hidden" value="true"><br>
  <input type="submit" value="Apply">
</form>
 </td>
</tr>
</table>
';
	if ( ($_SESSION["guest_login"] == 0) && $HTTP_GET_VARS["showctrl"] ) {
		echo $add_server_form;
	}
?>

&nbsp;
<table border=0 align=center cellpadding=4 cellspacing=0 width="100%">
<tr>
 <td class="smallheader" colspan=5 style="background-color: #000000"><b>Server</b></td>
</tr>
<tr>
 <td valign=middle class="server-header"><b>Status</b></td>
 <td valign=middle class="server-header"><b>Server name</b></td>
 <td valign=middle class="server-header"><b>users</b></td>
 <td valign=middle class="server-line"><a href="servers.php?cmd=disconnect&amp;ip=0&amp;port=0">Disconnect</a></td>
 <td valign=middle class="server-header"><a href="servers.php?showctrl=1">Server Preferences</a></td>
</tr>
<tr>
<?php
		echo '<td valign=middle class="server-line">';
		$stats = amule_get_stats();
		if ( $stats["id"] == 0 ) {
			echo "Not connected";
		} elseif ( $stats["id"] == 0xffffffff ) {
			echo "Connecting ...";
		} else {
			echo "Connected with ", (($stats["id"] < 16777216) ? "low ID" : "high ID");
		}
		echo '</td><td valign=middle class="server-line">', $stats["serv_name"], '</td>';
		echo '<td valign=middle class="server-line">', $stats["serv_users"], '</td>';
		
?>
 <td valign=middle class="server-header"><a href="servers.php?cmd=connect&amp;ip=0&amp;port=0">Connect to any server</a></td>
 <td valign=middle class="server-line"></td>
</tr>
</table>
&nbsp;
<table border=0 align=center cellpadding=4 cellspacing=0 width="100%">
<tr>
 <td class="smallheader" colspan=6 style="background-color: #000000"><b>Server list</b></td>
</tr>
<tr>
 <td valign=middle class="server-header-left"><a href="servers.php?sort=name"><b>Server name</b></a></td>
 <td valign=middle class="server-header"><a href="servers.php?sort=description"><b>Description</b></a></td>
 <td valign=middle class="server-header"><a href="servers.php?sort=ip"><b>IP</b></a></td>
 <td valign=middle class="server-header"><a href="servers.php?sort=users"><b>users</b></a></td>
 <td valign=middle class="server-header"><a href="servers.php?sort=files"><b>files</b></a></td>
 <td valign=middle class="server-header"><b>Actions</b></td>
</tr>

<?php
	if ( ($HTTP_GET_VARS["cmd"] != "") && ($_SESSION["guest_login"] == 0) ) {
		var_dump($HTTP_GET_VARS);
		if ( $HTTP_GET_VARS["cmd"] == "add" ) {
			amule_do_add_server_cmd($HTTP_GET_VARS["ip"], $HTTP_GET_VARS["port"], $HTTP_GET_VARS["name"]);
		} else {
			amule_do_server_cmd($HTTP_GET_VARS["ip"], $HTTP_GET_VARS["port"], $HTTP_GET_VARS["cmd"]);
		}
	}
	
	$servers = amule_load_vars("servers");

	foreach ($servers as $srv) {
		echo "<tr>";
		echo '<td valign=middle class="server-line-left">', $srv->name, '</td>';
		echo '<td valign=middle class="server-line">', $srv->desc, '</td>';
		echo '<td valign=middle class="server-line">', $srv->addr, '</td>';
		echo '<td valign=middle class="server-line">', $srv->users, '(', $srv->maxusers, ')', '</td>';
		echo '<td valign=middle class="server-line">', $srv->files, '</td>';
		if ( $_SESSION["guest_login"] == 0 ) {
			echo '<td valign=middle class="server-line"><acronym title="Connect">',
			'<a href="servers.php?cmd=connect&ip=', $srv->ip, '&port=', $srv->port,
			'" style="text-decoration: none"><img src="l_connect.gif" alt="Connect"></a></acronym>',
			'<acronym title="Remove selected server"><a href="servers.php?cmd=remove&ip=',
			$srv->ip, '&port=', $srv->port,
			"\" onclick=\"return confirm('Are you sure to remove this server from list?')\">",
			'<img src="l_cancel.gif" alt="Remove selected server"></a></acronym>';
		}
		echo "</tr>";
	}
?>
</table>
 </td>
</tr>
</table>
</font></body>
</html>
