<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
<META HTTP-EQUIV="PRAGMAS" CONTENT="NO-CACHE">
<?php
	echo "<title>aMule " , amule_get_version(), " - Web Control Panel</title>";
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
		<a href="stat_tree.php">
			<img src="cp_stats.gif"><br>
  			Statistics</a>
  		<font color="#000000">|</font>

  		<a href="stat_graphs.php">Graphs</a>
  	</td>
  	</td>
  	<td align="center" class="tabs" width="95">
  		<a href="preferences.php">
  			<img src="cp_settings.gif"><br>
  			Preferences
  		</a>
  	</td>

  	<td class="tabs" align="center" width="30">&nbsp;</td>
  	<td align="left" class="tabs" width="95">
  		<img src="log.gif"> <a href="index.php?serverinfo=1">Serverinfo</a><br>
  		<img src="log.gif"> <a href="index.php?log=1">Log</a>
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

	<script type="text/javascript" language="javascript">
	var d = new Date();
	s = "[ " + d.getDate() + "/" + (d.getMonth() + 1) + "/" + d.getFullYear() + " " + d.getHours() + ":" + (d.getMinutes() < 10 ? "0" : "") + d.getMinutes() + ":" + (d.getSeconds() < 10 ? "0" : "") + d.getSeconds() + " ]";
	document.write(s);
</script>
  </font>
 </td>
 <td align=right class=tabs>
  <form>
  <input type="button" value="ed2k://Download" onClick='self.location.href="index.php?links=1"'>
  <input type="button" value="Logout" onClick='self.location.href="login.html"'>
  </form>
 </td>
</tr>
</table>

&nbsp;
<p align=center><div class="message"></div></p><br>

<table border=0 align=center cellpadding=4 cellspacing=0 width="85%">
<tr>
<td valign=middle class="websearch-header"><b><font face=Tahoma style="font-size:10pt;">Search Results</font></b>&nbsp;&nbsp;(<a align="right" href="javascript:self.location.href='search.php'"><font face=Tahoma style="font-size:8pt;">Refetch Results</font></a>)</td>
</tr>

<tr>
<td valign=middle class="websearch-line">

<form action="" method="GET">
<table border=0 align=center cellpadding=4 cellspacing=0>
<input type="hidden" name=cmd value="download">
<tr>
<td class="websearch-line"><b><font face=Tahoma style="font-size:8pt;"><a href="search.php?sort=name">File Name</a></font></b></td>
<td class="websearch-line" width=50><b><font face=Tahoma style="font-size:8pt;"><a href="search.php?sort=size">Size</a></font></b></td>
<td class="websearch-line"><b><font face=Tahoma style="font-size:8pt;"><a href="search.php?sort=sources">Sources</a></b></td>
<td class="websearch-line" width=100><b><font face=Tahoma style="font-size:8pt;">Download</font></b></td>
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
		function cat2idx($cat)
		{
                	$cats = amule_get_categories();
                	$result = 0;
                	foreach($cats as $i => $c) {
                		if ( $cat == $c) $result = $i;
                	}
            		return $result;
		}

		//
		// declare it here, before any function reffered it in "global"
		//
		$sort_order;$sort_reverse;

		function my_cmp($a, $b)
		{
			global $sort_order, $sort_reverse;
			
			switch ( $sort_order) {
				case "size": $result = $a->size > $b->size; break;
				case "name": $result = $a->name > $b->name; break;
				case "sources": $result = $a->sources > $b->sources; break;
			}

			if ( $sort_reverse ) {
				$result = !$result;
			}

			return $result;
		}

		if ($_SESSION["guest_login"] == 0) {
			if ( $HTTP_GET_VARS["cmd"] == "search") {

				$search_type = -1;
				switch($HTTP_GET_VARS["method"]) {
					case "server": $search_type = 0; break;
					case "global": $search_type = 1; break;
					case "kad": $search_type = 2; break;
				}
	
				$min_size = $HTTP_GET_VARS["min"] == "" ? 0 : $HTTP_GET_VARS["min"];
				$max_size = $HTTP_GET_VARS["max"] == "" ? 0 : $HTTP_GET_VARS["max"];
	
				$min_size *= 1024*1024;
				$max_size *= 1024*1024;
				
				amule_do_search_start_cmd($HTTP_GET_VARS["tosearch"],
					$HTTP_GET_VARS["ext"], $HTTP_GET_VARS["type"],
					$search_type, $HTTP_GET_VARS["avail"], $min_size, $max_size);
			} elseif ( $HTTP_GET_VARS["cmd"] == "download") {
				foreach ( $HTTP_GET_VARS as $name => $val) {
					// this is file checkboxes
					if ( (strlen($name) == 32) and ($val == "on") ) {
						$cat = $HTTP_GET_VARS["cat"];
						$cat_idx = cat2idx($cat);
						amule_do_search_download_cmd($name, $cat_idx);
					}
				}
			} else {
				// wrong command come
				//var_dump($HTTP_GET_VARS);
			}
		}
		
		$search = amule_load_vars("searchresult");

		$sort_order = $HTTP_GET_VARS["sort"];

		if ( $sort_order == "" ) {
			$sort_order = $_SESSION["search_sort"];
		} else {
			if ( $_SESSION["search_sort_reverse"] == "" ) {
				$_SESSION["search_sort_reverse"] = 0;
			} else {
				$_SESSION["search_sort_reverse"] = !$_SESSION["search_sort_reverse"];
			}
		}

		$sort_reverse = $_SESSION["search_sort_reverse"];
		if ( $sort_order != "" ) {
			$_SESSION["search_sort"] = $sort_order;
			usort(&$search, "my_cmp");
		}

		foreach ($search as $file) {
			echo '<tr><td align="left"><font face=Tahoma style="font-size:8pt;">', $file->name,
				'</font></td><td class="websearch-line"><font face=Tahoma style="font-size:8pt;">', CastToXBytes($file->size),
				'</font></td><td class="websearch-line"><font face=Tahoma style="font-size:8pt;">', $file->sources,
				'</font></td><td class="websearch-line"><input type="checkbox" name="', $file->hash,
				'"></td></tr>';
				
		}
?>

</table>

<input type="submit" value="Download">
<img src="arrow_right.gif" align="absmiddle">
	<form>
	<select name="cat" size="1">
        <?php
        	$cats = amule_get_categories();
        	foreach($cats as $c) {
        		echo "<option>", $c, "</option>";
        	}
        ?>
	</select>
	</form>
	
</form>
<br>
<br>
&nbsp;
</table>
<br>

<form action="" method="GET">
<table border=0 align=center cellpadding=4 cellspacing=0 width="250">
<tr>
<td valign=middle class="websearch-header"><b><font face=Tahoma style="font-size:10pt;">Search</font></b></td>
</tr>
<tr>
 <td valign=middle class="websearch-line">
  <table>
  <tr><td class=tabs width=150>Name</td><td><input name="tosearch" type="text" size="40"></td></tr>
  <tr><td class=tabs>Type</td>
  <td>
  <select name="type">
  	<option value=""></option>
  	<option value="Archives">Archive</option>
	<option value="Audio">Audio</option>
  	<option value="CD-Images">CD-Images</option>
  	<option value="Pictures">Pictures</option>
  	<option value="Programs">Programs</option>
  	<option value="Texts">Texts</option>
  	<option value="Videos">Video</option>
  </select>
  </td></tr>
  <tr><td class=tabs>Min Size (MB)</td><td><input name="min" type="text" size="10"></td></tr>
  <tr><td class=tabs>Max Size (MB)</td><td><input name="max" type="text" size="10"></td></tr>
  <tr><td class=tabs>Min Availability</td><td><input name="avail" type="text" size="10"></td></tr>
  <tr><td class=tabs>Extension</td><td><input name="ext" type="text" size="10"></td></tr>
  <tr><td class=tabs>Method</td><td><input value="server" type="radio" name="method" checked>Server</td></tr>
  <tr><td class=tabs></td><td><input value="global" type="radio" name="method">Global Search</td></tr>
  <tr><td class=tabs></td><td><input value="kad" type="radio" name="method">Kad Search</td></tr>
  </table>
  <br>
  <input type="hidden" name=cmd value="search">
  <input type="submit" value="Search">
  <br>
  &nbsp;
</td>
</tr>
</table>


</body>
</html>
