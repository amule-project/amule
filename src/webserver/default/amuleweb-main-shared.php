<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

<link href="style.css" rel="stylesheet" type="text/css">
<script language="JavaScript" type="text/JavaScript">
function formCommandSubmit(command)
{
	<?php
		if ($_SESSION["guest_login"] != 0) {
				echo 'alert("You logged in as guest - commands are disabled");';
				echo "return;";
		}
	?>
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

</script>
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
    <td colspan="2"><form name="mainform" action="amuleweb-main-shared.php" method="post">
              <table class="center-table">
                <tr>
                  <td><input type="hidden" name="command"></td>
                  
            <td><a href="javascript:formCommandSubmit('reload');"><img src="images/refresh.png" alt="Reload shared files" name="reload" onload=""></a></td>
				  <td><a href="javascript:formCommandSubmit('prioup');"><img name="up" src="images/up.png" alt="Raise priority" onLoad=""></a></td>
                  
            <td><a href="javascript:formCommandSubmit('priodown');"><img src="images/down.png" alt="Lower priority" name="down" onload=""></a></td>
                  <td><select name="select">
                      <option selected>Select prio</option>
                      <option>Low</option>
                      <option>Normal</option>
                      <option>High</option>
                    </select> </td>
                  
            <td><a href="javascript:formCommandSubmit('setprio');"><img src="images/ok.png" alt="Set priority" name="resume" onload=""></a></td>
              
                  <td> 
                    <?php
		 	if ($_SESSION["guest_login"] != 0) {
				echo "<b>&nbsp;You logged in as guest - commands are disabled</b>";
			}
		 ?>
                  </td>
                </tr>
              </table>
        <table class="tab">
          <caption>
        SHARED FILES 
        </caption>
          <tr> 
            <td><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td>&nbsp;</td>
            <td><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td>&nbsp;</td>
            
          <td>
              <table class="w100p">
                <tr> 
                  <th></th>
                  <th><a href="amuleweb-main-shared.php?sort=name">File Name</a></th>
                  <th><a href="amuleweb-main-shared.php?sort=xfer">Transferred</a> 
                    (<a href="amuleweb-main-shared.php?sort=xfer_all">Total</a>)</th>
                  <th><a href="amuleweb-main-shared.php?sort=req">Requested</a> 
                    (<a href="amuleweb-main-shared.php?sort=req_all">Total</a>)</th>
                  <th><a href="amuleweb-main-shared.php?sort=acc">Accepted requests</a> 
                    (<a href="amuleweb-main-shared.php?sort=acc_all">Total</a>)</th>
                  <th><a href="amuleweb-main-shared.php?sort=size">Size</a></th>
                  <th><a href="amuleweb-main-shared.php?sort=prio">Priority</a></th>
                </tr><tr><td colspan="9" class="sep-dark"></td></tr>
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

		function PrioString($file)
		{
			$prionames = array(0 => "Low", 1 => "Normal", 2 => "High",
				3 => "Very high", 4 => "Very low", 5=> "Auto", 6 => "Release");
			$result = $prionames[$file->prio];
			if ( $file->prio_auto == 1) {
				$result = $result . "(auto)";
			}
			return $result;
		}

		function PrioSort($file) {
		// Very low (4) has a too high number
			if (4 == $file->prio) {
				return 0;
			}
			return $file->prio+1;
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
				case "xfer": $result = $a->xfer > $b->xfer; break;
				case "xfer_all": $result = $a->xfer_all > $b->xfer_all; break;
				case "acc": $result = $a->accept > $b->accept; break;
				case "acc_all": $result = $a->accept_all > $b->accept_all; break;
				case "req": $result = $a->req > $b->req; break;
				case "req_all": $result = $a->req_all > $b->req_all; break;
				case "prio": $result = PrioSort($a) < PrioSort($b); break;
			}

			if ( $sort_reverse ) {
				$result = !$result;
			}
			//var_dump($sort_reverse);
			return $result;
		}

		//
		// perform command before processing content
		//
		//var_dump($HTTP_GET_VARS);
		if (($HTTP_GET_VARS["command"] != "") && ($_SESSION["guest_login"] == 0)) {
			//amule_do_download_cmd($HTTP_GET_VARS["command"]);
			foreach ( $HTTP_GET_VARS as $name => $val) {
				// this is file checkboxes
				if ( (strlen($name) == 32) and ($val == "on") ) {
					//var_dump($name);var_dump($val);
					amule_do_shared_cmd($name, $HTTP_GET_VARS["command"]);
				}
			}
			if ($HTTP_GET_VARS["command"] == "reload") {
				amule_do_reload_shared_cmd();
			}
		}
		$shared = amule_load_vars("shared");

		// Whitelist against the column keys my_cmp() actually understands
		// (the switch() above). Anything not in the list is dropped to "",
		// which falls through to the "no sort change" branch below.
		// This prevents an attacker-controlled value from being stored in
		// $_SESSION["shared_sort"] and later reflected into rendered HTML
		// if any future template change prints that variable (#869 follow-up).
		// Plain string-equality chain is the only matching primitive the
		// bundled PHP interpreter supports (no in_array(), no htmlspecialchars()).
		$sort_raw = isset($HTTP_GET_VARS["sort"]) ? $HTTP_GET_VARS["sort"] : "";
		if ($sort_raw == "size" || $sort_raw == "name" || $sort_raw == "xfer" ||
		    $sort_raw == "xfer_all" || $sort_raw == "acc" || $sort_raw == "acc_all" ||
		    $sort_raw == "req" || $sort_raw == "req_all" || $sort_raw == "prio") {
			$sort_order = $sort_raw;
		} else {
			$sort_order = "";
		}

		if ( $sort_order == "" ) {
			$sort_order = $_SESSION["shared_sort"];
		} else {
			if ( $_SESSION["sort_reverse"] == "" ) {
				$_SESSION["sort_reverse"] = 0;
			} else {
				$_SESSION["sort_reverse"] = !$_SESSION["sort_reverse"];
			}
		}
		//var_dump($_SESSION);
		$sort_reverse = $_SESSION["sort_reverse"];
		if ( $sort_order != "" ) {
			$_SESSION["shared_sort"] = $sort_order;
			usort(&$shared, "my_cmp");
		}

		foreach ($shared as $file) {
			print "<tr>";

			echo "<td class='texte'>", '<input type="checkbox" name="', $file->hash, '" >', "</td>";

			echo "<td class='texte texte-full-name'>", $file->name, "</td>";
			echo "<td class='texte al-center'>", CastToXBytes($file->xfer), " (", CastToXBytes($file->xfer_all),")</td>";

			echo "<td class='texte al-center'>", $file->req, " (", $file->req_all, ")</td>";
			echo "<td class='texte al-center'>", $file->accept, " (", $file->accept_all, ")</td>";
			
			echo "<td class='texte al-center'>", CastToXBytes($file->size), "</td>";

			echo "<td class='texte al-center'>", PrioString($file), "</td>";;

			print "</tr><tr><td colspan='9' class='sep-light'></td></tr>";
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
        </table></form></td>
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
