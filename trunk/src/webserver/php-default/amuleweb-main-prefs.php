<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule preferences page</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
<style type="text/css">
<!--
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background-color: #003399;
}
-->
</style>
</head>
<script language="JavaScript" type="text/JavaScript">
function formCommandSubmit(command)
{
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

var initvals = new Object;

<?php
	$opts = amule_get_options();
	$opt_groups = array("connection", "files");
	var_dump($opt_groups);
	foreach ($opt_groups as $group) {
		$curr_opts = $opts[$group];
		var_dump($curr_opts);
		foreach ($curr_opts as $opt_name => $opt_val) {
			echo 'initvals["', $opt_name, '"] = "', $opt_val, '";';
		}
	}
?>

<!-- Assign php generated data to controls -->
function init_data()
{
	var frm = document.forms.mainform

	var str_param_names = new Array(
		"max_line_down_cap", "max_line_up_cap",
		"max_up_limit", "max_down_limit", "max_file_src",
		"slot_alloc", "max_conn_total",
		"tcp_port", "udp_port",
		"min_free_space")
	for(i = 0; i < str_param_names.length; i++) {
		frm[str_param_names[i]].value = initvals[str_param_names[i]];
	}
	var check_param_names = new Array(
		"autoconn_en", "reconn_en", "udp_en",
		"aich_trust", "alloc_full", "alloc_full_chunks",
		"check_free_space", "extract_metadata", "ich_en",
		"new_files_auto_dl_prio", "new_files_auto_ul_prio"
		)
	for(i = 0; i < check_param_names.length; i++) {
		frm[check_param_names[i]].checked = initvals[check_param_names[i]] == "1" ? true : false;
	}
}

</script>

<body onload="init_data();" >
<form name="mainform" action="amuleweb-main-prefs.php" method="post">
<table width="800" border="0" cellpadding="0" cellspacing="0">
  <!--DWLayoutTable-->
  <tr>
    <td width="52" height="40">&nbsp;</td>
    <td width="696">&nbsp;</td>
    <td width="52">&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="58">&nbsp;</td>
    <td><table width="100%" bgcolor="#66CC00" >
        <tr>
          <td colspan="5"><strong>Line capacity (for statistics only) </strong></td>
        </tr>
        <tr>
          <td width="18%">Max download rate </td>
          <td width="11%"><input name="max_line_down_cap" type="text" id="max_line_down_cap" size="4"></td>
          <td width="16%">Max upload rate </td>
          <td width="19%"><input name="max_line_up_cap" type="text" id="max_line_up_cap" size="4"></td>
          <td width="36%">&nbsp;</td>
        </tr>
    </table></td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="58">&nbsp;</td>
    <td><table width="100%" bgcolor="#66CC00" >
            <tr>
              <td colspan="5"><strong>Bandwidth limits</strong></td>
            </tr>
            <tr>
              <td width="18%">Max download rate
              </td>
              <td width="11%"><input name="max_down_limit" type="text" id="max_down_limit" size="4"></td>
              <td width="16%">Max upload rate                  </td>
              <td width="19%"><input name="max_up_limit" type="text" id="max_up_limit" size="4"></td>
              <td width="36%">Slot allocation
                <input name="slot_alloc" type="text" id="slot_alloc" size="4"></td>
      </tr>
    </table></td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="106">&nbsp;</td>
    <td><table width="100%" bgcolor="#66CC00" >
        <tr>
          <td colspan="5"><strong>Connection settings </strong></td>
        </tr>
        <tr>
          <td width="30%">Max total connections (total) </td>
          <td width="11%"><input name="max_conn_total" type="text" id="max_conn_total" size="4"></td>
          <td width="20%">Max sources per file </td>
          <td width="18%"><input name="max_file_src" type="text" id="max_file_src" size="4"></td>
          <td width="21%">&nbsp;</td>
        </tr>
        <tr>
          <td colspan="2"><input name="autoconn_en" type="checkbox" id="autoconn_en">
            Autoconnect at startup </td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
        </tr>
        <tr>
          <td colspan="2"><input name="reconn_en" type="checkbox" id="reconn_en">
            Reconnect when connection lost            </td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
          <td>&nbsp;</td>
        </tr>
    </table></td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="62">&nbsp;</td>
    <td><table width="100%" bgcolor="#66CC00" >
      <tr>
        <td colspan="5"><strong>Network settings </strong></td>
      </tr>
      <tr>
        <td width="13%">TCP port </td>
        <td width="23%"><input name="tcp_port" type="text" id="tcp_port" size="4"></td>
        <td width="12%">UDP port </td>
        <td width="17%"><input name="udp_port" type="text" id="udp_port" size="4"></td>
        <td width="35%"><input name="udp_en" type="checkbox" id="udp_en">
          Enable UDP connections          </td>
      </tr>
    </table></td>
    <td>&nbsp;</td>
  </tr>
  <tr valign="top">
    <td height="108">&nbsp;</td>
    <td><table width="100%" bgcolor="#66CC00" >
      <tr>
        <td colspan="4"><strong>File settings </strong></td>
      </tr>
      <tr>
        <td width="27%">Minimum free space (Mb)</td>
        <td width="15%"><input name="min_free_space" type="text" id="min_free_space" size="4"></td>
        <td width="3%">&nbsp;</td>
        <td width="51%"><input name="check_free_space" type="checkbox" id="check_free_space">
          Check free space          </td>
      </tr>
      <tr>
        <td colspan="2"><input name="new_files_auto_dl_prio" type="checkbox" id="new_files_auto_dl_prio">
    Added download files have auto priority</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td colspan="2"><input name="new_files_auto_ul_prio" type="checkbox" id="new_files_auto_ul_prio">
          New shared files have auto priority</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td colspan="2"><input name="ich_en" type="checkbox" id="ich_en">
          I.C.H. active</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td colspan="2"><input name="aich_trust" type="checkbox" id="aich_trust">
          AICH trusts every hash (not recommended)</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td colspan="2"><input name="alloc_full_chunks" type="checkbox" id="alloc_full_chunks">
          Alloc full chunks of .part files</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td colspan="2"><input name="alloc_full" type="checkbox" id="alloc_full">
          Alloc full disk space for .part files</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td colspan="2"><input name="new_files_paused" type="checkbox" id="new_files_paused">
          Add files to download queue in pause mode</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td colspan="2"><input name="extract_metadata" type="checkbox" id="extract_metadata">
          Extract metadata tags</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
    </table></td>
    <td>&nbsp;</td>
  </tr>
</table>
</form>
</body>
</html>
