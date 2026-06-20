<!doctype html>
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<?php
	// Auto-refresh: reload on a timer, but skip while any checkbox is
	// checked so the user's selection isn't wiped.
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<script type=\"text/JavaScript\">
	setInterval(function() {
		if (document.querySelectorAll('input[type=\"checkbox\"]:checked').length > 0) {
			return;
		}
		location.reload();
	}, 1000 * ", $_SESSION["auto_refresh"], ");
</script>";
	}
?>

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

function selectAll(check)
{
	var checkboxes = document.querySelectorAll('input[type="checkbox"]');
	checkboxes.forEach(function(checkbox) {
		checkbox.checked = check.checked;
	});
}

</script>
</head>
<body class="main">
<table class="page">
  <tr> 
    <td class="logo-cell"><a href="amuleweb-main-dload.php" title="Home"><img src="images/logo.png" width="143" height="64" alt="aMule"></a></td>
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
                  
            <td><a href="javascript:formCommandSubmit('reload');" title="Reload shared files"><img src="images/refresh.png" alt="Reload shared files" name="reload" onload=""></a></td>
				  <td><a href="javascript:formCommandSubmit('prioup');" title="Raise priority of selected files"><img name="up" src="images/up.png" alt="Raise priority" onLoad=""></a></td>

            <td><a href="javascript:formCommandSubmit('priodown');" title="Lower priority of selected files"><img src="images/down.png" alt="Lower priority" name="down" onload=""></a></td>
                  <td><select name="select">
                      <option selected>Change priority</option>
                      <option>Auto</option>
                      <option>Very low</option>
                      <option>Low</option>
                      <option>Normal</option>
                      <option>High</option>
                      <option>Very high</option>
                      <option>Release</option>
                    </select> </td>
                  
            <td><a href="javascript:formCommandSubmit('setprio');" title="Apply selected priority"><img src="images/ok.png" alt="Set priority" name="resume" onload=""></a></td>
              
                  <td> 
                    <?php
		 	if ($_SESSION["guest_login"] != 0) {
				echo "<b>&nbsp;You logged in as guest - commands are disabled</b>";
			}
		 ?>
                  </td>
                </tr>
              </table>
        <table class="w100p"><tr><td class="h10"></td></tr></table>
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
                  <th class="al-left"><input type="checkbox" name="selectAllFiles" onclick="selectAll(this)"></th>
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
			// Emit the raw byte count; the unit formatting is done
			// client-side (see the js-size script at the end of the page).
			return '<span class="js-size">' . $size . '</span>';
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
			$cmd = $HTTP_GET_VARS["command"];
			// Dropdown label -> priority constant (src/Constants.h)
			$prio_values = array("Low" => 0, "Normal" => 1, "High" => 2,
				"Very high" => 3, "Very low" => 4, "Auto" => 5, "Release" => 6);
			foreach ( $HTTP_GET_VARS as $name => $val) {
				// this is file checkboxes
				if ( (strlen($name) == 32) and ($val == "on") ) {
					//var_dump($name);var_dump($val);
					if ( $cmd == "setprio" ) {
						$sel = $HTTP_GET_VARS["select"];
						if ( isset($prio_values[$sel]) ) {
							amule_do_shared_cmd($name, "prio", $prio_values[$sel]);
						}
					} else {
						amule_do_shared_cmd($name, $cmd);
					}
				}
			}
			if ($cmd == "reload") {
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

			echo "<td class='texte texte-full-name'>", htmlspecialchars($file->name), "</td>";
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
<script type="text/JavaScript">
// Format the raw byte counts emitted by the backend (spans with class
// "js-size") into human-readable units. Done here in the browser because
// the webserver's PHP interpreter lacks sprintf/round.
function formatBytes(value) {
	var b = parseFloat(value);
	if ( isNaN(b) ) return value;
	if ( b < 1024 ) return b + " Bytes";
	if ( b < 1048576 ) return (b / 1024).toFixed(2) + " KB";
	if ( b < 1073741824 ) return (b / 1048576).toFixed(2) + " MB";
	return (b / 1073741824).toFixed(2) + " GB";
}
(function() {
	var els = document.getElementsByClassName("js-size");
	for ( var i = 0; i < els.length; i++ ) {
		els[i].textContent = formatBytes(els[i].textContent);
	}
})();
</script>
</body>
</html>
