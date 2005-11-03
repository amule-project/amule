<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
  <head>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
	<meta http-equiv="pragmas" content="no-cache">
    <title>
      aMule CVS - Web Control Panel
    </title>
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"],
			'; url=downloads.php', '">';
	}
?>
<style type="text/css">
  img {
    border : 0px;
  }

  a, a:link, a:visited {
    color : white;
    text-decoration: none;
  }

  a:hover {
    color: #FFC412;
    text-decoration: none;
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
  .websearch-header, .websearch-line, .addserver-header, .addserver-line {

    font-family : Tahoma;
    font-size : 8pt;
  }

  .tabs {
    font-family : Tahoma;
    font-size : 10pt;
    background-color : #1F76A5;
  }

  .tabs_too {
    font-family : Tahoma;
    font-size : 10pt;
    background-color : #0075B3;
  }

  .tabs_three {
    font-family : Tahoma;
    font-size : 10pt;
  }

  .tabs_four {
    font-family : Tahoma;
    font-size : 10pt;
    background-color : #21719B;
  }

  .tabs_five {
    font-family : Tahoma;
    font-size : 10pt;
    background-color : #1D6083;
  }

  .down-header, .down-line, .down-line-good, .up-header, .up-line,
  .server-header, .server-line, .shared-header, .shared-line, .shared-line-changed,
  .upqueue-header, .upqueue-line,
  .websearch-header, .websearch-line, .addserver-header, .addserver-line {

    text-align : center;
  }

  .down-header-left, .down-line-left, .down-line-good-left,
  .server-header-left, .server-line-left, .shared-header-left,
  .up-header-left, .up-line-left, .shared-line-left, .shared-line-left-changed, .upqueue-line-left {

    text-align : left;
  }

  .down-line-right, .down-line-good-right, .down-header-right {
    text-align : right;
  }

  .down-header, .down-header-left, .down-header-right,
  .up-header, .up-header-left, .server-header, .server-header-left,
  .shared-header, .shared-header-left, .upqueue-header,
  .websearch-header, .addserver-header {

    background-color : #1D6083;
  }

  .header {
    background-color : #0046AC;
  }

  .smallheader {
    background-color : #003399;
    color : #FFFFFF;
  }

  .commontext {
    background-color : #FFFFFF;
    color : #000000;
  }

  .commontext_too {
    color : #FFFFFF;
    font-family : Tahoma;
    font-size : 8pt;
  }

  .down-line, .down-line-good, .down-line-left, .down-line-good-left,
  .down-line-right, .down-line-good-right,
  .up-line, .up-line-left, .server-line, .server-line-left,
  .shared-line, .shared-line-changed, .shared-line-left, .shared-line-left-changed,
  .upqueue-line, .upqueue-line-left,
  .websearch-line, .addserver-line {

    background-color : #1F76A5;
  }

  .down-line-good, .down-line-good-left, .down-line-good-right,
  .shared-line-changed, .shared-line-left-changed {

    color : #F0F000;
  }

  .percent_table {
    border:0px solid #000000;
    border-collapse: collapse;
  }

  .message {
    font-family : Tahoma;
    font-size : 8pt;
    font-weight: bold;
    color: #FFFFFF;
    background-color: #1D6083;
  }

  .dinput {
    border-width: 1px;
    border-color: black;
  }

</style>
  </head>
  <body background="main_bg.gif" text=white link="#1F76A5" vlink="#1F76A5" alink="#1F76A5" marginwidth=0 marginheight=0 topmargin=0 leftmargin=0 style="margin:0px">
    <table border="0" width="100%" align="center" cellpadding="0" cellspacing="0">
      <tr>
       <td class="tabs_three" background="main_top_bg.gif" align="left" colspan="4">

        <table border="0" cellpadding="4" cellspacing="0" width="100%">
        <tr>
          <td class="tabs_three" align="center" width="100">

            &nbsp;
            <font face="Tahoma" style="font-size:13pt;" color="#000000"><a href="http://www.amule.org" target="_blank">
              <img src="emule.gif" alt="aMule | Web Control Panel">
            </a>
          </td>
          <td class="tabs_three" align="center" width="30">
            &nbsp;
          </td>
          <td align="center" class="tabs_three" width="95">

            <a href="servers.php">
              <img src="cp_servers.gif"><br />
              Server list
            </a>
          </td>
          <td align="center" class="tabs_three" width="96">
            <a href="downloads.php">
              <img src="cp_download.gif"><br />
              Transfer
            </a>

          </td>
          <td align="center" class="tabs_three" width="96">
            <a href="search.php">
              <img src="cp_search.gif"><br />
              Search
            </a>
          </td>
          <td align="center" class="tabs_three" width="96">
            <a href="shared.php">

              <img src="cp_shared.gif"><br />
              Shared Files
            </a>
          <td align="center" class="tabs_three" width="110">
            <a href="stat_tree.php">
              <img src="cp_stats.gif"><br />
              Statistics
            </a>
            <font color="#000000">|</font>

            <a href="stat_graphs.php">
              Graphs
            </a>
          </td>
          <td align="center" class="tabs_three" width="95">
            <a href="preferences.php">
              <img src="cp_settings.gif"><br />
              Preferences
            </a>
          </td>

          <td class="tabs_three" align="center">
            &nbsp;
          </td>
          <td align="left" class="tabs_three" width="95">
            <img src="log.gif" align="absmiddle"> <a href="#" onClick="self.location.href='?ses=1070417518&amp;w=sinfo'">Serverinfo</a><br />
            <img src="log.gif" align="absmiddle"> <a href="#" onClick="self.location.href='?ses=1070417518&amp;w=log#end'">Log</a><!--<br />
            <img src="log.gif" align="absmiddle"> <a href="#" onClick="self.location.href='?ses=1070417518&amp;w=debuglog#end'">Debug Log</a>-->
          </td>

        </tr>
        </table>

       </td>
      </tr>
      <tr>
       <td background="main_topbar.gif" height="49" class="tabs_four">
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
         <font color="#FFE471">
           <script language="javascript">
             var d = new Date();
             s = "[ " + d.getDate() + "/" + (d.getMonth() + 1) + "/" + d.getFullYear() + " " + d.getHours() + ":" + (d.getMinutes() < 10 ? "0" : "") + d.getMinutes() + ":" + (d.getSeconds() < 10 ? "0" : "") + d.getSeconds() + " ]";
             document.write(s);
           </script>
         </font>
       </td>
       <td background="main_topbar.gif" align="center" valign="middle" class="tabs_four">

        <a href="index.php?links=1">
          <img src="arrow_right.gif" align="absmiddle">
          &nbsp; ed2k:// ED2K Link(s)
        </a>
       </td>
       <td background="main_topbar.gif" align="right">
       
        <table border="0" cellpadding="0" cellspacing="0" width="8" height="100%">
           <tr>
             <td height="49" background="main_topbarseperator.gif">

               &nbsp;
             </td>
           </tr>
         </table>
         
       </td>
       <td background="main_topbardarker.gif" align="center" valign="middle" class="tabs_five">
         <a href="login.html">
           <img src="arrow_down_logout.gif" align="absmiddle">
           &nbsp; Logout
         </a>

       </td>
      </tr>
    </table><font face=Tahoma style="font-size:8pt;">
</font>
&nbsp;<table border=0 align=center cellpadding=4 cellspacing=0 width="95%">
<tr>
 <td align=center valign=middle>

<script type="text/javascript">

function GotoCat(cat) {
	window.location.href="downloads.php?cmd=filter&status="+cat;
}

</script>

<font face=Tahoma style="font-size:8pt;">
<table border=0 align=center cellpadding=3 cellspacing=0 width="100%" bgcolor="#99CCFF">

<tr>
 <td class="smallheader" style="background-color: #000000" colspan="8"><img src="arrow_down.gif">
 <b>Downloads
 <?php
 	$downloads = amule_load_vars("downloads");
 	echo '&nbsp;(', count($downloads), ')';
 ?>
 </b></td>
 <td align="right" class="smallheader" style="background-color: #000000">
 <form>
 	<select name="cat" size="1" onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)>
	<?php
    	$all_status = array("All", "Waiting", "Paused", "Downloading");
    	
		if ( $HTTP_GET_VARS["cmd"] == "filter") {
			$_SESSION["filter_status"] = $HTTP_GET_VARS["status"];
		}
    	if ( $_SESSION["filter_status"] == '') $_SESSION["filter_status"] = 'All';
    	foreach ($all_status as $s) {
    		echo (($s == $_SESSION["filter_status"]) ? '<option selected>' : '<option>'), $s, '</option>';
    	}
	?>
 	</select>
 </form>
 </td>
</tr>
<tr>
 <td valign=middle class="down-header-left"><a href="downloads.php?sort=name"><b>File Name</b></a></td>

 <td valign=middle class="down-header"><a href="downloads.php?sort=size"><b>Size</b></a></td>
 <td valign=middle class="down-header"><a href="downloads.php?sort=completed"><b>Complete</b></a></td>
 <td valign=middle class="down-header"><a href="downloads.php?sort=transferred"><b>Transferred</b></a></td>
 <td valign=middle class="down-header"><a href="downloads.php?sort=progress"><b>Progress</b></a></td>
 <td valign=middle class="down-header"><a href="downloads.php?sort=speed"><b>Speed</b></a></td>
 <td valign=middle class="down-header"><b>Sources</b></td>

 <td valign=middle class="down-header"><b>Priority</b></td>
 <td valign=middle class="down-header"><b>Actions</b></td>
</tr>

<?php
	function CastToXBytes($size)
	{
		if ( $size < 1024 ) {
			$result = $size . " byte";
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

	if ( ($HTTP_GET_VARS["cmd"] != "") && ($_SESSION["guest_login"] == 0) ) {
		$name = $HTTP_GET_VARS['file'];
		if ( strlen($name) == 32 ) {
			amule_do_download_cmd($name, $HTTP_GET_VARS["cmd"]);
		}
	}
	
	$downloads = amule_load_vars("downloads");

	foreach ($downloads as $file) {
		$status = StatusString($file);
		if ( ($_SESSION['filter_status'] == 'All') || ($_SESSION['filter_status'] == $status) ) {
			echo '<tr>';
			$mark = $file->speed > 0 ? 1 : 0;
			echo '<td valign=top class="', $mark ? 'down-line-good-left':'down-line-left',
				'"><acronym title="', $file->name, '">', $file->short_name, '</acronym></td>';
			echo '<td valign=top class="', $mark ? 'down-line-good-right':'down-line-right','">',
				CastToXBytes($file->size), '</td>';
			echo '<td valign=top class="', $mark ? 'down-line-good-right':'down-line-right','">',
				CastToXBytes($file->size_done), '</td>';
			echo '<td valign=top class="', $mark ? 'down-line-good-right':'down-line-right','">',
				CastToXBytes($file->size_xfer), '</td>';
			
			echo '<td valign=middle class="', $mark ? 'down-line-right">':'down-line">';
			echo '<table width=200 height=11 border=1 class="percent_table" cellpadding=0 cellspacing=0 bordercolor="#000000">';
			echo '<tr><td><img src="greenpercent.gif" height=4 width=', (($file->size_done * 1.0)/$file->size)*200 + 1, '><br>';
			echo $file->progress, '</td></tr></table></td>';
	
			echo '<td valign=middle class="', $mark ? 'down-line-good-right">':'down-line-right">',
				$file->speed ? (CastToXBytes($file->speed) . '/s') : '-', '</td>';
	
			// source count
			echo '<td valign=middle class="', $mark ? 'down-line-good-right">':'down-line-right">';
			if ( $file->src_count_not_curr != 0 ) {
				echo $file->src_count - $file->src_count_not_curr, "&nbsp;/&nbsp;";
			}
			echo $file->src_count, "&nbsp;(", $file->src_count_xfer, ")";
			if ( $file->src_count_a4af != 0 ) {
				echo "+", $file->src_count_a4af;
			}
			echo '</td>';
	
			echo '<td valign=middle class="', $mark ? 'down-line-good-right">':'down-line-right">',
					PrioString($file), '</td>';
	
			echo '<td valign=top class="down-line"><acronym title="', $status, '">';
			echo '<img src="l_info.gif" alt="', $status, '"></acronym>';
	
			// commands
			echo '<acronym title="ED2K Link(s)"><a href="', $file->link, '"><img src="l_ed2klink.gif" alt="ED2K Link(s)"></a></acronym>';
			if ( $_SESSION["guest_login"] == 0 ) {
				if ( $file->status == 7 ) {
					echo '<acronym title="Resume"><a href="?cmd=resume&file=', $file->hash, '"><img src="l_resume.gif" alt="Resume"></a></acronym>';
				} else {
					echo '<acronym title="Pause"><a href="?cmd=pause&file=', $file->hash, '"><img src="l_pause.gif" alt="Pause"></a></acronym>';
				}
				echo '<acronym title="Cancel"><a href="?cmd=cancel&file=', $file->hash,
					"\" onclick=\"return confirm('Are you sure that you want to cancel and delete this file?')\" ",
					'"><img src="l_cancel.gif" alt="Cancel"></a></acronym>';
				echo '<acronym title="Increase priority"><a href="?cmd=pause&file=', $file->hash, '"><img src="l_up.gif" alt="Increase priority"></a></acronym>';
				echo '<acronym title="Decrease priority"><a href="?cmd=pause&file=', $file->hash, '"><img src="l_down.gif" alt="Decrease priority"></a></acronym>';
			}
	
			echo '</tr>', "\n";
		}
	}
?>

</table>
&nbsp;
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
<tr>

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
<p align=center>


&nbsp;
</p>
</font>
</td>
</tr>
</table></body>
</html>
