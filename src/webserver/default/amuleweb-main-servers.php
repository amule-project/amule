<!doctype html>
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

<link href="style.css" rel="stylesheet" type="text/css">
</head>
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
    <td colspan="2">
        <table class="tab">
          <caption>
        SERVERS 
        </caption>
          <tr> 
            <td><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td>&nbsp;</td>
            <td><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            
      <td><table class="w100p">
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
                <th class="w3p"></th>
                <th class="w22p" ><a href="amuleweb-main-servers.php?sort=name">Server Name</a></th>
                <th class="w42p" ><a href="amuleweb-main-servers.php?sort=desc">Description</a></th>
                <th class="w19p">Address</th>
                <th class="w7p"><a href="amuleweb-main-servers.php?sort=users">Users</a></th>
                <th class="w7p"><a href="amuleweb-main-servers.php?sort=files">Files</a></th>
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

			echo "<td class='texte'>", htmlspecialchars($srv->name), "</td>";
			echo "<td class='texte'>", htmlspecialchars($srv->desc), "</td>";
			echo "<td class='texte al-center'>", htmlspecialchars($srv->addr), "</td>";
			echo "<td class='texte al-center'>", $srv->users, "</td>";
			echo "<td class='texte al-center'>", $srv->files, "</td>";

			echo "</tr><tr><td colspan='9' class='sep-light'></td></tr>";
		}
	  ?>
            </table></td>
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
