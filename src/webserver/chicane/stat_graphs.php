<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
  <head>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <META HTTP-EQUIV="PRAGMAS" CONTENT="NO-CACHE">
    <title>
      aMule CVS - Web Control Panel
    </title>
    
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
            <img src="log.gif" align="absmiddle"> <a href="index.php?serverinfo=1">Serverinfo</a><br />
            <img src="log.gif" align="absmiddle"> <a href="index.php?log=1">Log</a>
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
    </table>
&nbsp;

<?php
	// reload graphs data
	amule_load_vars("stats_graph");
?>

<table border=0 align=center cellpadding=4 cellspacing=0>
<tr>
<td class="websearch-line"><img src="amule_stats_download.png" width="500" height="200" border="0" alt="" title="" /></td>
</tr>
<tr>
<td width="600" class="websearch-line"><b><font face=Tahoma style="font-size:8pt;">Download speed</font></b></td>
</tr>
</table>
&nbsp;
<table border=0 align=center cellpadding=4 cellspacing=0>
<tr>
<td class="websearch-line"><img src="amule_stats_upload.png" width="500" height="200" border="0" alt="" title="" /></td>
</tr>
<tr>
<td width="600" class="websearch-line"><b><font face=Tahoma style="font-size:8pt;">Upload speed</font></b></td>
</tr>
</table>
&nbsp;
<table border=0 align=center cellpadding=4 cellspacing=0>
<tr>
<td class="websearch-line"><img src="amule_stats_conncount.png" width="500" height="200" border="0" alt="" title="" /></td>
</tr>
<tr>
<td width="600" class="websearch-line"><b><font face=Tahoma style="font-size:8pt;">Number of connections</font></b></td>
</tr>
</table>

  </body>
</html>
