<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
<META HTTP-EQUIV="PRAGMAS" CONTENT="NO-CACHE">
<title>aMule CVS - Web Control Panel</title>
<meta http-equiv=refresh content="100; ">
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
		<a href="#" onClick="self.location.href='?ses=1962921590&amp;w=stats'; return false;">
			<img src="cp_stats.gif"><br>
  			Statistics</a>
  		<font color="#000000">|</font>

  		<a href="#" onClick="self.location.href='?ses=1962921590&amp;w=graphs'; return false;">Graphs</a>
  	</td>
  	</td>
  	<td align="center" class="tabs" width="95">
  		<a href="#" onClick="self.location.href='?ses=1962921590&amp;w=options'; return false;">
  			<img src="cp_settings.gif"><br>
  			Preferences
  		</a>
  	</td>

  	<td class="tabs" align="center" width="30">&nbsp;</td>
  	<td align="left" class="tabs" width="95">
  		<img src="log.gif"> <a href="#" onClick="self.location.href='?ses=1962921590&amp;w=sinfo'; return false;">Serverinfo</a><br>
  		<img src="log.gif"> <a href="#" onClick="self.location.href='?ses=1962921590&amp;w=log#end'; return false;">Log</a><!--<br>
  		<img src="log.gif"> <a href="#" onClick="self.location.href='?ses=1962921590&amp;w=debuglog#end'; return false;">Debug Log</a>-->
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
  <form><input type="button" name="queue" value="Logout" onClick="self.location.href='?ses=1962921590&amp;w=logout'"></form>
 </td>
</tr>
</table><font face=Tahoma style="font-size:8pt;">
</font>
&nbsp;<table border=0 align=center cellpadding=4 cellspacing=0 width="95%">

<tr>
 <td align=center valign=middle>

<script type="text/javascript">
function GotoCat(cat) {
	var loc= window.location.href;
	var pos= loc.indexOf("cat=");

	if (pos>1) {
		if (loc.substr(pos-1,1)=="&") pos--;
		var subs=loc.substr(pos+3,256);
		var pos2=subs.indexOf("&");
		if (pos2==-1) pos2=loc.length;
		pos2 += pos+3;
		var t1=loc.substring(0,pos);
		var t2=loc.substring(pos2+1,loc.length );
		loc=t1+t2;
	}
	window.location.href=loc+"&cat="+cat;
}
</script><font face=Tahoma style="font-size:8pt;">
<table border=0 align=center cellpadding=3 cellspacing=0 width="100%" bgcolor="#99CCFF">
<tr>
 <td class="smallheader" style="background-color: #000000" colspan="8"><img src="arrow_down.gif">
 <b>Downloads
 <?php
 	$downloads = amule_load_vars("downloads");
 	echo '&nbsp;(', count($downloads), ')';
 ?>
 </b></td>
 <td align="right" class="smallheader" style="background-color: #000000"><form><select name="cat" size="1"onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)><option selected value="">All others</option><option value="Waiting">Waiting</option><option value="Downloading">Downloading</option><option value="Erroneous">Erroneous</option><option value="Paused">Paused</option><option value="Stopped">Stopped</option></select></form></td>

</tr>
<tr>
 <td valign=middle class="down-header-left"><a href="?sort=name"><b>File Name</b></a></td>
 <td valign=middle class="down-header"><a href="?sort=size"><b>Size</b></a></td>
 <td valign=middle class="down-header"><a href="?sort=completed"><b>Complete</b></a></td>
 <td valign=middle class="down-header"><a href="?sort=transferred"><b>Transferred</b></a></td>
 <td valign=middle class="down-header"><a href="?sort=progress"><b>Progress</b></a></td>

 <td valign=middle class="down-header"><a href="?sort=speed"><b>&nbsp;&nbsp;&nbsp;&nbsp;Speed&nbsp;&nbsp;&nbsp;&nbsp;</b></a></td>
 <td valign=middle class="down-header"><b>Sources</b></td>
 <td valign=middle class="down-header"><b>Priority</b></td>
 <td valign=middle class="down-header"><b>Actions</b></td>
</tr>

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
	function PrioString($file)
	{
		$prionames = array(0 => "Low", 1 => "Normal", 2 => "High",
			3 => "Very high", 4 => "Very low", 5=> "Auto", 6 => "Powershare");
		$result = $prionames[$file->prio];
		if ( $file->prio_auto == 1) {
			$result = $result . "(auto)";
		}
		return $result;
	}
	function StatusString($file)
	{
		if ( $file->status == 7 ) {
			return "Paused";
		} elseif ( $file->src_count_xfer > 0 ) {
			return "Downloading";
		} else {
			return "Waiting";
		}
	}

	$downloads = amule_load_vars("downloads");

	foreach ($downloads as $file) {
		echo '<td valign=top class="down-line-left"><acronym title="', $file->name, '">', $file->short_name, '</acronym></td>';
		echo '<td valign=top class="down-line-right">', CastToXBytes($file->size), '</td>';
		echo '<td valign=top class="down-line-right">', CastToXBytes($file->size_done), '</td>';
		echo '<td valign=top class="down-line-right">', CastToXBytes($file->size_xfer), '</td>';
		
		echo '<td valign=middle class="down-line">';
		echo '<table width=200 height=11 border=1 class="percent_table" cellpadding=0 cellspacing=0 bordercolor="#000000">';
		echo '<tr><td><img src="greenpercent.gif" height=4 width=', 60, '><br>';
		echo $file->progress, '</td></tr></table></td>';
		
		echo '<td valign=top class="down-line-right">', CastToXBytes($file->speed), '</td>';
		
		// source count
		echo '<td valign=top class="down-line">';
		if ( $file->src_count_not_curr != 0 ) {
			echo $file->src_count - $file->src_count_not_curr, "&nbsp;/&nbsp;";
		}
		echo $file->src_count, "&nbsp;(", $file->src_count_xfer, ")";
		if ( $file->src_count_a4af != 0 ) {
			echo "+", $file->src_count_a4af;
		}
		echo '</td>';

		echo '<td valign=top class="down-line">', PrioString($file), '</td>';

		$status = StatusString($file);
		echo '<td valign=top class="down-line"><acronym title="', $status, '">';
		echo '<img src="l_info.gif" alt="', $status, '"></acronym>';
		
		// commands
		echo '<acronym title="ED2K Link(s)"><a href="', $file->link, '"><img src="l_ed2klink.gif" alt="ED2K Link(s)"></a></acronym>';
		echo '<acronym title="Pause"><a href="?cmd=pause&file=', $file->hash, '"><img src="l_pause.gif" alt="Pause"></a></acronym>';
		echo '<acronym title="Cancel"><a href="?cmd=cancel&file=', $file->hash,
			"\" onclick=\"return confirm('Are you sure that you want to cancel and delete this file?')\" ",
			'"><img src="l_cancel.gif" alt="Cancel"></a></acronym>';
		echo '<acronym title="Increase priority"><a href="?cmd=pause&file=', $file->hash, '"><img src="l_up.gif" alt="Increase priority"></a></acronym>';
		echo '<acronym title="Decrease priority"><a href="?cmd=pause&file=', $file->hash, '"><img src="l_down.gif" alt="Decrease priority"></a></acronym>';
		echo '</tr>';
		echo "\n";
	}
?>
<tr>
</tr>
</table>
<table border=0 align=center cellpadding=4 cellspacing=0 width="100%">
<tr>

 <td class="smallheader" colspan=4 style="background-color: #000000"><img src="arrow_up.gif">
 <b>Uploads
 <?php
 	$uploads = amule_load_vars("uploads");
 	echo '&nbsp;(', count($uploads), ')';
 ?>
 </b></td>

</tr>
<tr>
 <td class="up-header-left"><b>Username</b></td>
 <td class="up-header"><b>File Name</b></td>
 <td class="up-header"><b>Transferred</b></td>
 <td class="up-header"><b>Speed</b></td>
</tr>
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
 	$uploads = amule_load_vars("uploads");
 	foreach ($uploads as $file) {
 		echo '<tr>';
 		echo '<td valign=top class="up-line-left"><acronym title="', $file->user_name, '">', $file->user_name, '</acronym></td>';
 		echo '<td valign=top class="up-line-left"><acronym title="', $file->name, '">', $file->short_name, '</acronym></td>';
 		echo '<td valign=top class="up-line">', CastToXBytes($file->xfer_up), "&nbsp;/&nbsp;", CastToXBytes($file->xfer_down), '</td>';
 		echo '<td valign=top class="up-line">', ($file->xfer_speed > 0) ? (CastToXBytes($file->xfer_speed) . "/s") : "-", '</td>';
 		echo '</tr>';
		echo "\n";
 	}
 ?>
</table>
&nbsp;
<p align=center>&nbsp;
  </p>
</font>
</td>
</tr>
</table></body>
</html>
