<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}

	// Kad network controls. Dispatch before stats are rendered so the
	// page shown after the submit reflects the post-action state.
	// Note: amule's mini PHP parser tokenises only "<<=" / ">>=", not
	// "<<" / ">>", and has no intval() builtin -- so the IP packing
	// uses arithmetic (* 16777216 etc.) and relies on the runtime's
	// implicit string-to-number coercion via the arithmetic operators.
	if ($_SESSION["guest_login"] == 0) {
		$kad_action = $HTTP_GET_VARS["kad_action"];
		if ($kad_action == "connect_known") {
			amule_kad_start();
		} elseif ($kad_action == "connect_ip") {
			$ip0 = $HTTP_GET_VARS["ip0"] + 0;
			$ip1 = $HTTP_GET_VARS["ip1"] + 0;
			$ip2 = $HTTP_GET_VARS["ip2"] + 0;
			$ip3 = $HTTP_GET_VARS["ip3"] + 0;
			$port = $HTTP_GET_VARS["port"] + 0;
			// Pack the dotted-quad with ip0 (template's high-octet
			// field) in the high byte and ip3 in the low byte.
			$packed = $ip0 * 16777216 + $ip1 * 65536 + $ip2 * 256 + $ip3;
			if ($packed != 0 and $port != 0) {
				amule_kad_connect($packed, $port);
			}
		} elseif ($kad_action == "update_url") {
			$nodes_url = $HTTP_GET_VARS["nodes_url"];
			if ($nodes_url != "") {
				amule_kad_update_from_url($nodes_url);
			}
		} elseif ($kad_action == "disconnect") {
			amule_kad_disconnect();
		}
	}

	amule_load_vars("stats_graph");
?>
<link href="style.css" rel="stylesheet" type="text/css">
</head><script language="JavaScript" type="text/JavaScript">
function formCommandSubmit(command)
{
	if ( command == "cancel" ) {
		var res = confirm("Delete selected files ?")
		if ( res == false ) {
			return;
		}
	}
	if ( command != "filter" ) {
		<?php
			if ($_SESSION["guest_login"] != 0) {
					echo 'alert("You logged in as guest - commands are disabled");';
					echo "return;";
			}
		?>
	}
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

</script>
<body class="main">
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
    <td colspan="2"><form name="mainform" action="amuleweb-main-kad.php" method="post">
        <table class="tab">
          <caption>
          KADEMLIA 
          </caption>
          <tr> 
            <td><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td>&nbsp;</td>
            <td><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            <td><table class="w100p">
                <tr class="va-top">
                  <td class="h200"><img src="amule_stats_kad.png" width="500" height="200" alt="" title="" /></td>
                  <td class="va-top"> <table class="kadnewnode">
                      <tr>
                        <th colspan="2">Network</th>
                      </tr>
                      <tr>
                        <td colspan="2" class="al-center">
                          <button type="submit" name="kad_action" value="connect_known">Connect from known peers</button>
                          &nbsp;
                          <button type="submit" name="kad_action" value="disconnect">Disconnect</button>
                        </td>
                      </tr>
                      <tr>
                        <th colspan="2">Bootstrap from node</th>
                      </tr>
                      <tr>
                        <td class="al-right">IP :</td><td class="al-left"><input name="ip3" type="text" id="ip32" size="3" maxlength="3">
                          &nbsp; <input name="ip2" type="text" id="ip23" size="3" maxlength="3">
                          &nbsp; <input name="ip1" type="text" id="ip13" size="3" maxlength="3">
                          &nbsp; <input name="ip0" type="text" id="ip03" size="3" maxlength="3"></td>
                      </tr>
                      <tr>
                        <td class="al-right">Port :</td><td class="al-left"><input name="port" type="text" id="port3" size="4" maxlength="5">
                          &nbsp; <button type="submit" name="kad_action" value="connect_ip">Connect</button></td>
                      </tr>
                      <tr>
                        <th colspan="2">Update bootstrap from URL</th>
                      </tr>
                      <tr>
                        <td class="al-right">URL :</td><td class="al-left"><input name="nodes_url" type="text" id="nodes_url" size="32">
                          &nbsp; <button type="submit" name="kad_action" value="update_url">Update</button></td>
                      </tr>
                    </table></td>
                </tr>
                <tr class="va-top"> 
                  <td class="h20 al-center w500">Number of nodes</td>
                  <td></td>
                </tr>
              </table></td>
            <td>&nbsp;</td>
          </tr>
          <tr> 
            <td><img src="images/tab_bottom_left.png" width="24" height="24"></td>
            <td>&nbsp;</td>
            <td><img src="images/tab_bottom_right.png" width="24" height="24"></td>
          </tr>
        </table>
        </form></td>
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
