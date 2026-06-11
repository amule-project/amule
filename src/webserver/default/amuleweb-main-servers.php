<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

<link href="style.css" rel="stylesheet" type="text/css">
</head>
<body class="main">
<table width="100%" height="100%" cellpadding="0" cellspacing="0">
  <tr class="va-top"> 
    <td width="143" class="logo-cell"><img src="images/logo.png" width="143" height="64"></td>
    <td width="100%" class="navbar-cell"> <table class="navbar-table" cellspacing="0" cellpadding="0">
        <tr> 
          <td><a class="navbutton nav-transfer" href="amuleweb-main-dload.php" title="Transfers"></a></td>
          <td><a class="navbutton nav-shared" href="amuleweb-main-shared.php" title="Shared files"></a></td>
          <td><a class="navbutton nav-search" href="amuleweb-main-search.php" title="Search"></a></td>
          <td><a class="navbutton nav-servers" href="amuleweb-main-servers.php" title="Servers"></a></td>
          <td><a class="navbutton nav-kad" href="amuleweb-main-kad.php" title="Kad"></a></td>
          <td><a class="navbutton nav-stats" href="amuleweb-main-stats.php" title="Statistics"></a></td>
          <td><img src="images/col.png"></td>
          <td width="10"></td>
          <td width="190" class="texteinv al-right"><a href="login.php">exit</a><br> 
            <a href="amuleweb-main-log.php">log &bull;</a> <a href="amuleweb-main-prefs.php">configuration</a></td>
          <td width="10"></td>
        </tr>
      </table></td>
  </tr>
  <tr class="al-center va-top"> 
    <td colspan="2">
        <table width="100%" cellspacing="0" cellpadding="0">
          <caption>
        SERVERS 
        </caption>
          <tr> 
            <td width="24"><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td class="tab-top">&nbsp;</td>
            <td width="24"><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td width="24" class="tab-left">&nbsp;</td>
            
      <td class="bg-white"><table width="100%"  cellpadding="0" cellspacing="0">
              <?php
                // Network-level disconnect button, admin only. Statements
                // must be syntactically complete within one PHP block;
                // see the other guest gates further down in this file
                // for the same pattern. Inline form + button to match the
                // Kad page styling and give a proper clickable button
                // rather than a plain text link.
                if ($_SESSION["guest_login"] == 0) {
                    echo '<tr><td colspan="6" class="al-right" style="padding:4px 8px;">',
                         '<form action="amuleweb-main-servers.php" method="get" style="display:inline;">',
                         '<input type="hidden" name="server_action" value="disconnect">',
                         '<button type="submit">Disconnect from current ed2k server</button>',
                         '</form>',
                         '</td></tr>';
                }
              ?>
              <tr>
                <th width="3%"></th>
                <th width="22%" ><a href="amuleweb-main-servers.php?sort=name">Server Name</a></th>
                <th width="42%" ><a href="amuleweb-main-servers.php?sort=desc">Description</a></th>
                <th width="19%">Address</th>
                <th width="7%"><a href="amuleweb-main-servers.php?sort=users">Users</a></th>
                <th width="7%"><a href="amuleweb-main-servers.php?sort=files">Files</a></th>
		</tr><tr><td colspan="8" class="sep-dark"></td></tr>
              <?php


		//
		// declare it here, before any function reffered it in "global"
		//
		$sort_order;$sort_reverse;

		function my_cmp($a, $b)
		{
			global $sort_order, $sort_reverse;
			switch ( $sort_order) {
				case "name": $result = $a->name > $b->name; break;
				case "desc": $result = $a->desc > $b->desc; break;
				case "users": $result = $a->users > $b->users; break;
				case "files":$result = $a->files > $b->files; break;
			}

			if ( $sort_reverse ) {
				$result = !$result;
			}
			return $result;
		}

		$servers = amule_load_vars("servers");

		// Whitelist against the column keys my_cmp() actually understands
		// (the switch() above). Anything not in the list is dropped to "",
		// which falls through to the "no sort change" branch below (#869 follow-up).
		$sort_raw = isset($HTTP_GET_VARS["sort"]) ? $HTTP_GET_VARS["sort"] : "";
		if ($sort_raw == "name" || $sort_raw == "desc" ||
		    $sort_raw == "users" || $sort_raw == "files") {
			$sort_order = $sort_raw;
		} else {
			$sort_order = "";
		}

		//
		// perform command before processing content
		//
		if ( ($HTTP_GET_VARS["cmd"] != "") and ($HTTP_GET_VARS["ip"] != "") and ($HTTP_GET_VARS["port"] != "")) {
			if ($_SESSION["guest_login"] == 0) {
				amule_do_server_cmd($HTTP_GET_VARS["ip"], $HTTP_GET_VARS["port"], $HTTP_GET_VARS["cmd"]);
			}
		}
		// Network-level disconnect (no per-server target). The
		// amule_do_server_cmd path above always carries an ip/port
		// tag, so it can only target one server at a time.
		if ($HTTP_GET_VARS["server_action"] == "disconnect") {
			if ($_SESSION["guest_login"] == 0) {
				amule_server_disconnect();
			}
		}
		
		if ( $sort_order == "" ) {
			$sort_order = $_SESSION["servers_sort"];
		} else {
			if ( $_SESSION["sort_reverse"] == "" ) {
				$_SESSION["sort_reverse"] = 0;
			} else {
				$_SESSION["sort_reverse"] = !$_SESSION["sort_reverse"];
			}
		}

		$sort_reverse = $_SESSION["sort_reverse"];
		if ( $sort_order != "" ) {
			$_SESSION["servers_sort"] = $sort_order;
			usort(&$servers, "my_cmp");
		}
		foreach ($servers as $srv) {
			echo "<tr>";
			
			if ($_SESSION["guest_login"] != 0) {
				echo "<td class='texte al-center'></td>";
			} else {
				echo "<td class='texte al-center'>",
					'<a href="amuleweb-main-servers.php?cmd=connect&ip=', $srv->ip,
					'&port=', $srv->port, '">',
					'<img src="images/connect.gif" width="16" height="16">','</a>',
					'<a href="amuleweb-main-servers.php?cmd=remove&ip=', $srv->ip,
					'&port=', $srv->port, '">',
					'<img src="images/cancel.gif" width="16" height="16">','</a>',
					"</td>";
			}

			echo "<td class='texte'>", $srv->name, "</td>";
			echo "<td class='texte'>", $srv->desc, "</td>";
			echo "<td class='texte al-center'>", $srv->addr, "</td>";
			echo "<td class='texte al-center'>", $srv->users, "</td>";
			echo "<td class='texte al-center'>", $srv->files, "</td>";

			echo "</tr><tr><td colspan='9' class='sep-light'></td></tr>";
		}
	  ?>
            </table></td>
            <td width="24" class="tab-right">&nbsp;</td>
          </tr>
          <tr> 
            <td width="24"><img src="images/tab_bottom_left.png" width="24" height="24"></td>
            <td class="tab-bottom">&nbsp;</td>
            <td width="24"><img src="images/tab_bottom_right.png" width="24" height="24"></td>
          </tr>
        </table></td>
  </tr>
  <tr class="va-bottom"> 
    <td class="h25" colspan="2"> <table width="100%" height="40" cellpadding="0" cellspacing="0">
        <tr class="al-center va-middle"> 
          <td width="50%"> <iframe name="stats" src="footer.php" height="35" width="100%" scrolling="no" frameborder="0">edklink</iframe> 
          </td>
          <td width="50%"> <iframe name="stats" src="stats.php" height="35" width="100%" scrolling="no" frameborder="0">connection</iframe> 
          </td>
        </tr>
      </table></td>
  </tr>
</table>
</body>
</html>
