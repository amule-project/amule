<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

<link href="style.css" rel="stylesheet" type="text/css">
<script language="JavaScript" type="text/JavaScript">
function formCommandSubmit(command)
{
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

var initvals = new Object;

<?php
	// apply new options before proceeding
	//var_dump($HTTP_GET_VARS);
	if ( ($HTTP_GET_VARS["Submit"] == "Apply") && ($_SESSION["guest_login"] == 0) ) {
		$file_opts = array("check_free_space", "extract_metadata", 
			"ich_en","aich_trust", "preview_prio","save_sources", "resume_same_cat",
			"min_free_space", "new_files_paused", "alloc_full", "alloc_full_chunks",
			"new_files_auto_dl_prio", "new_files_auto_ul_prio"
		);
		$conn_opts = array("max_line_up_cap","max_up_limit",
			"max_line_down_cap","max_down_limit", "slot_alloc",
			"tcp_port","udp_port","udp_dis","max_file_src","max_conn_total","autoconn_en","reconn_en",
			"network_ed2k","network_kad");
		$webserver_opts = array("use_gzip", "autorefresh_time");

		$all_opts;
		foreach ($conn_opts as $i) {
			$curr_value = $HTTP_GET_VARS[$i];
			if ( $curr_value == "on") $curr_value = 1;
			if ( $curr_value == "") $curr_value = 0;

			$all_opts["connection"][$i] = $curr_value;
		}
		foreach ($file_opts as $i) {
			$curr_value = $HTTP_GET_VARS[$i];
			if ( $curr_value == "on") $curr_value = 1;
			if ( $curr_value == "") $curr_value = 0;

			$all_opts["files"][$i] = $curr_value;
		}
		foreach ($webserver_opts as $i) {
			$curr_value = $HTTP_GET_VARS[$i];
			if ( $curr_value == "on") $curr_value = 1;
			if ( $curr_value == "") $curr_value = 0;

			$all_opts["webserver"][$i] = $curr_value;
		}
		// general: nickname is a single top-level string, not nested.
		$nick_value = $HTTP_GET_VARS["nick"];
		if ( $nick_value != "" ) {
			$all_opts["nick"] = $nick_value;
		}
		//var_dump($all_opts);
		amule_set_options($all_opts);
	}
	$opts = amule_get_options();
	//var_dump($opts);
	// These values are emitted into double-quoted JS string literals inside
	// the <script> block, so they are escaped with addslashes (JS-string
	// context), not htmlspecialchars (which would show literal entities).
	echo 'initvals["nick"] = "', addslashes($opts["nick"]), '";';
	$opt_groups = array("connection", "files", "webserver");
	//var_dump($opt_groups);
	foreach ($opt_groups as $group) {
		$curr_opts = $opts[$group];
		//var_dump($curr_opts);
		foreach ($curr_opts as $opt_name => $opt_val) {
			echo 'initvals["', addslashes($opt_name), '"] = "', addslashes($opt_val), '";';
		}
	}
?>

<!-- Assign php generated data to controls -->
function init_data()
{
	var frm = document.forms.mainform

	var str_param_names = new Array(
		"nick",
		"max_line_down_cap", "max_line_up_cap",
		"max_up_limit", "max_down_limit", "max_file_src",
		"slot_alloc", "max_conn_total",
		"tcp_port", "udp_port",
		"min_free_space",
		"autorefresh_time"
		)
	for(i = 0; i < str_param_names.length; i++) {
		frm[str_param_names[i]].value = initvals[str_param_names[i]];
	}
	var check_param_names = new Array(
		"autoconn_en", "reconn_en", "udp_dis", "new_files_paused",
		"aich_trust", "alloc_full", "alloc_full_chunks",
		"check_free_space", "extract_metadata", "ich_en",
		"new_files_auto_dl_prio", "new_files_auto_ul_prio",
		"use_gzip",
		"network_ed2k", "network_kad"
		)
	for(i = 0; i < check_param_names.length; i++) {
		frm[check_param_names[i]].checked = initvals[check_param_names[i]] == "1" ? true : false;
	}
}

</script>
</head>
<body class="main" onLoad="init_data();">
<table class="page">
  <tr> 
    <td class="logo-cell"><img src="images/logo.png" width="143" height="64"></td>
    <td class="navbar-cell"> <table class="navbar-table">
        <tr> 
          <td><a class="navbutton nav-transfer" href="amuleweb-main-dload.php" title="Transfers"></a></td>
          <td><a class="navbutton nav-shared" href="amuleweb-main-shared.php" title="Shared files"></a></td>
          <td><a class="navbutton nav-search" href="amuleweb-main-search.php" title="Search"></a></td>
          <td><a class="navbutton nav-servers" href="amuleweb-main-servers.php" title="Servers"></a></td>
          <td><a class="navbutton nav-kad" href="amuleweb-main-kad.php" title="Kad"></a></td>
          <td><a class="navbutton nav-stats" href="amuleweb-main-stats.php" title="Statistics"></a></td>
          <td><img src="images/col.png"></td>
          <td></td>
          <td><a href="login.php">exit</a><br> 
            <a href="amuleweb-main-log.php">log &bull;</a> <a href="amuleweb-main-prefs.php">configuration</a></td>
          <td></td>
        </tr>
      </table></td>
  </tr>
  <tr> 
    <td colspan="2">
        <table class="tab">
          <caption>PREFERENCES</caption>
          <tr> 
            <td><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td>&nbsp;</td>
            <td><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            
      <td><form name="mainform" action="amuleweb-main-prefs.php" method="post">
              <table class="prefs-grid">
               
    <tr>
      <td>
        <table class="prefs-pane">
          <tr>
            <td>&nbsp;</td>
            <th>General</th>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td>&nbsp;</td>
            <td>Nickname</td>
            <td>
<input name="nick" type="text" id="nick0" size="20"></td>
          </tr>
        </table>
        <table class="prefs-pane">
          <tr>
            <td>&nbsp;</td>
            <th>Webserver</th>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td>&nbsp;</td>
            <td>Page refresh interval </td>
            <td>
<input name="autorefresh_time" type="text" id="autorefresh_time7" size="4"></td>
          </tr>
          <tr> 
            <td>
<input name="use_gzip" type="checkbox" id="use_gzip5"></td>
            <td> Use gzip compression </td>
            <td>&nbsp;</td>
          </tr>
        </table></td>
      <td> 
        <table class="prefs-pane">
          <tr> 
            <td>&nbsp;</td>
            <th>Line capacity (for statistics only)</th>
            <td>&nbsp;</td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>Max download rate </td>
            <td>
<input name="max_line_down_cap" type="text" id="max_line_down_cap6" size="4"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>Max upload rate </td>
            <td>
<input name="max_line_up_cap" type="text" id="max_line_up_cap7" size="4"></td>
          </tr>
        </table></td>
    </tr>
    <tr> 
      <td> 
        <table class="prefs-pane">
          <tr> 
            <td class="h19">&nbsp;</td>
            <th>Bandwidth limits</th>
            <td>&nbsp;</td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>Max download rate </td>
            <td>
<input name="max_down_limit" type="text" id="max_down_limit6" size="4"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>Max upload rate </td>
            <td>
<input name="max_up_limit" type="text" id="max_up_limit6" size="4"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>Slot allocation </td>
            <td>
<input name="slot_alloc" type="text" id="slot_alloc6" size="4"></td>
          </tr>
        </table></td>
      <td rowspan="3"> 
        <table class="prefs-pane">
          <tr> 
            <td>&nbsp;</td>
            <th> File settings </th>
            <td>&nbsp;</td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>&nbsp; </td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="check_free_space" type="checkbox" id="check_free_space5"></td>
            <td> Check free space =&gt; Minimum free space (Mb) </td>
            <td> 
              <input name="min_free_space" type="text" id="min_free_space4" size="4"></td>
          </tr>
          <tr> 
            <td> 
              <input name="new_files_auto_dl_prio" type="checkbox" id="new_files_auto_dl_prio4"></td>
            <td> Added download files have auto priority</td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="new_files_auto_ul_prio" type="checkbox" id="new_files_auto_ul_prio4"></td>
            <td> New shared files have auto priority</td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="ich_en" type="checkbox" id="ich_en5"></td>
            <td> I.C.H. active</td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="aich_trust" type="checkbox" id="aich_trust4"></td>
            <td> AICH trusts every hash (not recommended)</td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="alloc_full_chunks" type="checkbox" id="alloc_full_chunks4"></td>
            <td> Alloc full chunks of .part files</td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="alloc_full" type="checkbox" id="alloc_full4"></td>
            <td> Alloc full disk space for .part files</td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="new_files_paused" type="checkbox" id="new_files_paused4"></td>
            <td> Add files to download queue in pause mode</td>
            <td></td>
          </tr>
          <tr> 
            <td> 
              <input name="extract_metadata" type="checkbox" id="extract_metadata4"></td>
            <td> Extract metadata tags </td>
            <td></td>
          </tr>
        </table></td>
    </tr>
    <tr> 
      <td> 
        <table class="prefs-pane">
          <tr> 
            <td>&nbsp;</td>
            <th>Connection settings</th>
            <td>&nbsp;</td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>Max total connections (total) </td>
            <td>
<input name="max_conn_total" type="text" id="max_conn_total8" size="4"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>Max sources per file </td>
            <td>
<input name="max_file_src" type="text" id="max_file_src7" size="4"></td>
          </tr>
          <tr> 
            <td>
<input name="autoconn_en" type="checkbox" id="autoconn_en6"></td>
            <td> Autoconnect at startup </td>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td>
<input name="reconn_en" type="checkbox" id="reconn_en6"></td>
            <td> Reconnect when connection lost </td>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td>
<input name="network_ed2k" type="checkbox" id="network_ed2k6"></td>
            <td> Enable ED2K network </td>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td>
<input name="network_kad" type="checkbox" id="network_kad6"></td>
            <td> Enable Kademlia network </td>
            <td>&nbsp;</td>
          </tr>
        </table></td>
    </tr>
    <tr>
      <td>
        <table class="prefs-pane">
          <tr>
            <td>&nbsp;</td>
            <th>Network settings</th>
            <td>&nbsp;</td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>TCP port </td>
            <td>
<input name="tcp_port" type="text" id="tcp_port6" size="4"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td>UDP port </td>
            <td>
<input name="udp_port" type="text" id="udp_port6" size="4"></td>
          </tr>
          <tr> 
            <td>
<input name="udp_dis" type="checkbox" id="udp_dis5"></td>
            <td> Disable UDP connections </td>
            <td>&nbsp;</td>
          </tr>
        </table></td>
    </tr>
    <tr> 
      <td colspan="2"> 
        <?php
			if ($_SESSION["guest_login"] == 0) {
				echo '<input type="submit" name="Submit" value="Apply">';
			} else {
				echo "<b>&nbsp;You can not change options - logged in as guest</b>";
			}
		?>
        <input name="command" type="hidden" id="command"> </td>
    </tr>
  </table>
  </form></td>
            <td>&nbsp;</td>
          </tr>
          <tr> 
            <td><img src="images/tab_bottom_left.png" width="24" height="24"></td>
            <td>&nbsp;</td>
            <td><img src="images/tab_bottom_right.png" width="24" height="24"></td>
          </tr>
        </table></td>
  </tr>
  <tr> 
    <td colspan="2"> <table class="footer-bar">
        <tr> 
          <td> <iframe name="stats" src="footer.php" height="35">edklink</iframe> 
          </td>
          <td> <iframe name="stats" src="stats.php" height="35">connection</iframe> 
          </td>
        </tr>
      </table></td>
  </tr>
</table>
</body>
</html>
