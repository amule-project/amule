<!doctype html>
<html>
<head>
<title>aMule control panel</title>
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}
?><meta http-equiv="Content-Type" content="text/html; charset=UTF-8">


<link href="style.css" rel="stylesheet" type="text/css">
<script language="JavaScript" type="text/JavaScript">
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
            <a href="amuleweb-main-log.php">log &bull;</a> <a href="amuleweb-main-prefs.php">configuration</a>
            </td>
          <td></td>
        </tr>
      </table></td>
  </tr>
  <tr> 
    <td colspan="2"><form action="amuleweb-main-dload.php" method="post" name="mainform">
        <table class="w100p">
          <tr>
      <td><table class="center-table">
          <tr> 
            <td><input type="hidden" name="command"></td>
            <td><a href="javascript:formCommandSubmit('pause');"><img name="pause" src="images/pause.png" alt="pause"></a></td>
            <td><a href="javascript:formCommandSubmit('resume');"><img name="resume" src="images/play.png" alt="resume"></a></td>
        		<td><a href="javascript:formCommandSubmit('prioup');"><img name="prioup" src="images/up.png" alt="prioup"></a></td>
        		<td><a href="javascript:formCommandSubmit('priodown');"><img name="priodown" src="images/down.png" alt="priodown"></a></td>
        		<td><a href="javascript:formCommandSubmit('cancel');"><img name="cancel" src="images/close.png" alt="cancel"></a></td>
      <td><table>
                <tr> 
                  <td> 
                    <?php
        	$all_status = array("all", "Waiting", "Paused", "Downloading");	
 			if ( $HTTP_GET_VARS["command"] == "filter") {
 				$_SESSION["filter_status"] = $HTTP_GET_VARS["status"];
 				$_SESSION["filter_cat"] = $HTTP_GET_VARS["category"];
 			}
        	if ( $_SESSION["filter_status"] == '') $_SESSION["filter_status"] = 'all';
        	if ( $_SESSION["filter_cat"] == '') $_SESSION["filter_cat"] = 'all';

        	echo '<select name="status"> ';
        	foreach ($all_status as $s) {
        		$label = ($s == 'all') ? 'All status' : $s;
        		$sel = ($s == $_SESSION["filter_status"]) ? ' selected' : '';
        		echo '<option value="', htmlspecialchars($s), '"', $sel, '>', htmlspecialchars($label), '</option>';
        	}
        	echo '</select>';
        	//var_dump($_SESSION["filter_cat"]);
        	echo '<select name="category" id="category">';
			$cats = amule_get_categories();
			foreach($cats as $c) {
				$label = ($c == 'all') ? 'All categories' : $c;
				$sel = ($c == $_SESSION["filter_cat"]) ? ' selected' : '';
				echo '<option value="', htmlspecialchars($c), '"', $sel, '>', htmlspecialchars($label), '</option>';
			}
			echo '</select>';
        ?>
                  </td>
                  			<td><a href="javascript:formCommandSubmit('filter');"><img src="images/filter.png" alt="Apply" name="resume"></a></td>
                  <td>&nbsp;</td>
                  <td>&nbsp;</td>
                  <td> 
                    <?php
		 	if ($_SESSION["guest_login"] != 0) {
				echo "<b>&nbsp;You logged in as guest - commands are disabled</b>";
			}
		 ?>
                  </td>
                </tr>
              </table></td>
          </tr>
        </table></td>
    </tr>
    <tr> 
      <td class="h10"> </td>
    </tr>
    <tr> 
      <td colspan="2"><table class="tab"> <caption>
          DOWNLOAD 
          </caption>
  <tr>
    <td><img src="images/tab_top_left.png" width="24" height="24"></td>
    <td>&nbsp;</td>
    <td><img src="images/tab_top_right.png" width="24" height="24"></td>
  </tr>
  <tr>
    <td>&nbsp;</td>
    <td><table class="w100p">
                     
          <tr> 
                  <th>&nbsp;</th>
                  <th><a href="amuleweb-main-dload.php?sort=name">File name</a></th>
                  <th><a href="amuleweb-main-dload.php?sort=size">Size</a></th>
                  <th><a href="amuleweb-main-dload.php?sort=size_done">Completed</a></th>
                  <th><a href="amuleweb-main-dload.php?sort=speed">Download speed</a></th>
                  <th><a href="amuleweb-main-dload.php?sort=progress">Progress</a></th>
                  <th><a href="amuleweb-main-dload.php?sort=srccount">Sources</a></th>
                  <th><a href="amuleweb-main-dload.php?sort=status">Status</a></th>
                  <th><a href="amuleweb-main-dload.php?sort=prio">Priority</a></th>
          </tr><tr><td colspan="9" class="sep-dark"></td></tr>
          <?php
		function CastToXBytes($size, &$count) {
			$count += $size;
			if ( $size < 1024 ) {
				$result = $size . " b";
			} elseif ( $size < 1048576 ) {
				$result = ($size / 1024.0) . " kb";
			} elseif ( $size < 1073741824 ) {
				$result = ($size / 1048576.0) . " mb";
			} else {
				$result = ($size / 1073741824.0) . " gb";
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
		
		//
		// declare it here, before any function reffered it in "global"
		//
		$sort_order;$sort_reverse;

		function my_cmp($a, $b)
		{
			global $sort_order, $sort_reverse;
			
			switch ( $sort_order) {
				case "size": $result = $a->size > $b->size; break;
				case "size_done": $result = $a->size_done > $b->size_done; break;
				case "progress": $result = (((float)$a->size_done)/((float)$a->size)) > (((float)$b->size_done)/((float)$b->size)); break;
				case "name": $result = $a->name > $b->name; break;
				case "speed": $result = $a->speed > $b->speed; break;
				case "srccount": $result = $a->src_count > $b->src_count; break;
				case "status": $result = StatusString($a) > StatusString($b); break;
				case "prio": $result = $a->prio < $b->prio; break;
			}

			if ( $sort_reverse ) {
				$result = !$result;
			}
			//var_dump($sort_reverse);
			return $result;
		}

		//
		// perform command before processing content

		if ( ($HTTP_GET_VARS["command"] != "") && ($_SESSION["guest_login"] == 0) ) {
			foreach ( $HTTP_GET_VARS as $name => $val) {
				// this is file checkboxes
				if ( (strlen($name) == 32) and ($val == "on") ) {
					//var_dump($name);
					amule_do_download_cmd($name, $HTTP_GET_VARS["command"]);
				}
			}
			//
			// check "filter-by-status" settings
			//
			if ( $HTTP_GET_VARS["command"] == "filter") {
				//var_dump($_SESSION);
				$_SESSION["filter_status"] = $HTTP_GET_VARS["status"];
				$_SESSION["filter_cat"] = $HTTP_GET_VARS["category"];
			}
		}
		if ( $_SESSION["filter_status"] == "") $_SESSION["filter_status"] = "all";
		if ( $_SESSION["filter_cat"] == "") $_SESSION["filter_cat"] = "all";
		$countSize = 0;
		$countCompleted = 0;
		$countSpeed = 0;
		$downloads = amule_load_vars("downloads");
		$fakevar=0;
		// Whitelist against the column keys my_cmp() actually understands
		// (the switch() above). Anything not in the list is dropped to "",
		// which falls through to the "no sort change" branch below.
		// This prevents an attacker-controlled value from being stored in
		// $_SESSION["download_sort"] and later reflected into rendered HTML
		// (#869 follow-up; same fix applied to shared and servers pages).
		$sort_raw = isset($HTTP_GET_VARS["sort"]) ? $HTTP_GET_VARS["sort"] : "";
		if ($sort_raw == "size" || $sort_raw == "size_done" || $sort_raw == "progress" ||
		    $sort_raw == "name" || $sort_raw == "speed" || $sort_raw == "srccount" ||
		    $sort_raw == "status" || $sort_raw == "prio") {
			$sort_order = $sort_raw;
		} else {
			$sort_order = "";
		}

		if ( $sort_order == "" ) {
			$sort_order = $_SESSION["download_sort"];
		} else {
			if ( $_SESSION["download_sort_reverse"] == "" ) {
				$_SESSION["download_sort_reverse"] = 0;
			} else {
				if ( $HTTP_GET_VARS["sort"] != '') {
					$_SESSION["download_sort_reverse"] = !$_SESSION["download_sort_reverse"];
				}
			}
		}
		//var_dump($_SESSION);
		$sort_reverse = $_SESSION["download_sort_reverse"];
		if ( $sort_order != "" ) {
			$_SESSION["download_sort"] = $sort_order;
			usort(&$downloads, "my_cmp");
		}

		//
		// Prepare categories index array
		$cats = amule_get_categories();
		foreach($cats as $i => $c) {
			$cat_idx[$c] = $i;
		}

		foreach ($downloads as $file) {
			$filter_status_result = ($_SESSION["filter_status"] == "all") or
				($_SESSION["filter_status"] == StatusString($file));
				
			$filter_cat_result = ($_SESSION["filter_cat"] == "all") or
				($cat_idx[ $_SESSION["filter_cat"] ] == $file->category);

			if ( $filter_status_result and $filter_cat_result) {
				print "<tr>";
	
				echo "<td class='texte h22'>", '<input type="checkbox" name="', $file->hash, '" >', "</td>";
	
				echo "<td class='texte texte-full-name h22'>", htmlspecialchars($file->name), "</td>";
				
				echo "<td class='texte h22 al-center'>", CastToXBytes($file->size, $countSize), "</td>";

				echo "<td class='texte h22 al-center'>", CastToXBytes($file->size_done, $countCompleted), "&nbsp;(",
					($file->size > 0) ? (((float)$file->size_done*100)/((float)$file->size)) : 0, "%)</td>";

				echo "<td class='texte h22 al-center'>", ($file->speed > 0) ? (CastToXBytes($file->speed, $countSpeed) . "/s") : "-", "</td>";

				echo "<td class='texte h22 al-center'>", $file->progress, "</td>";
	
				echo "<td class='texte h22 al-center'>";
				if ( $file->src_count_not_curr != 0 ) {
					echo $file->src_count - $file->src_count_not_curr, " / ";
				}
				echo $file->src_count, " ( ", $file->src_count_xfer, " ) ";
				if ( $file->src_count_a4af != 0 ) {
					echo "+ ", $file->src_count_a4af;
				}
				echo "</td>";
	
				echo "<td class='texte h22 al-center'>", StatusString($file), "</td>";
				
				echo "<td class='texte h22 al-center'>", PrioString($file), "</td>";
				
				print "</tr><tr><td colspan='9' class='sep-light'></td></tr>";
			}
		}
		if (count($downloads)>0) {
			echo "<tr>";
			echo "<td style='padding-bottom:0;'></td>";
			echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;text-align: right;padding-right: 20px;' class='h22 al-center'>Total</td>";
			echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;' class='h22 al-center'>", CastToXBytes($countSize, $fakevar), "</td>";
			echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;' class='h22 al-center'>", CastToXBytes($countCompleted, $fakevar), "&nbsp;(",
				($countSize > 0) ? (((float)$countCompleted*100)/((float)$countSize)) : 0, "%)</td>";
			echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;' class='h22 al-center'>", ($countSpeed > 0) ? (CastToXBytes($countSpeed, $fakevar) . "/s" ) : "", "</td>";
			echo "<td style='padding-bottom:0;'></td>";
			echo "<td style='padding-bottom:0;'></td>";
			echo "<td style='padding-bottom:0;'></td>";
			echo "<td style='padding-bottom:0;'></td>";
			echo "</tr>";
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
  </table>
</form>
      <table class="tab"><caption>
        UPLOAD 
        </caption>
        <tr> 
          <td><img src="images/tab_top_left.png" width="24" height="24"></td>
          <td>&nbsp;</td>
          <td><img src="images/tab_top_right.png" width="24" height="24"></td>
        </tr>
        <tr> 
          <td>&nbsp;</td>
          <td><table class="w100p">
              
        <tr> 
                <td>&nbsp;</td>
                <th>File Name</th>
                <th>Username</th>
                <th>Up</th>
                <th>Down</th>
                <th>&nbsp;</th>
                <th>&nbsp;</th>
                <th>Speed</th>
                <td>&nbsp;</td>
        </tr><tr><td colspan="9" class="sep-dark"></td></tr>
        <?php
			function CastToXBytes($size, &$count) {
				$count += $size;
				if ( $size < 1024 ) {
					$result = $size . " b";
				} elseif ( $size < 1048576 ) {
					$result = ($size / 1024.0) . " kb";
				} elseif ( $size < 1073741824 ) {
					$result = ($size / 1048576.0) . " mb";
				} else {
					$result = ($size / 1073741824.0) . " gb";
				}
				return $result;
			}
			$countUploadDimension = 0;
			$countDownloadDimension = 0;
			$countSpeed = 0;
			$uploads = amule_load_vars("uploads");
			$fakevar=0;
			foreach ($uploads as $file) {
				echo "<tr>";
	
				echo "<td class='texte h22 al-center'>", "</td>";
				
				echo "<td class='texte texte-full-name texte-full-name-upload h22'>", htmlspecialchars($file->name), "</td>";

				echo "<td class='texte h22 al-center'>", htmlspecialchars($file->user_name), "</td>";
	
				echo "<td class='texte h22 al-center'>", CastToXBytes($file->xfer_up, $countUploadDimension), "</td>";
				echo "<td class='texte h22 al-center'>", CastToXBytes($file->xfer_down, $countDownloadDimension), "</td>";
				echo "<td class='texte h22 al-center'>", "</td>";
				echo "<td class='texte h22 al-center'>", "</td>";
				echo "<td class='texte h22 al-center'>", ($file->xfer_speed > 0) ? (CastToXBytes($file->xfer_speed, $countSpeed) . "/s") : "-", "</td>";
				echo "<td class='texte h22 al-center'>", "</td>";
				echo "</tr><tr><td colspan='9' class='sep-light'></td></tr>";
			}
			if (count($uploads)>0) {
				echo "<tr>";
				echo "<td style='padding-bottom:0;'></td>";
				echo "<td style='padding-bottom:0;'></td>";
				echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;text-align: right;padding-right: 20px;' class='h22 al-center'>Total</td>";
				echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;' class='h22 al-center'>", CastToXBytes($countUploadDimension, $fakevar), "</td>";
				echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;' class='h22 al-center'>", CastToXBytes($countDownloadDimension, $fakevar), "</td>";
				echo "<td style='padding-bottom:0;'></td>";
				echo "<td style='padding-bottom:0;'></td>";
				echo "<td style='font-size:12px;color:#908c8c;padding-bottom:0;' class='h22 al-center'>", CastToXBytes($countSpeed, $fakevar) . "/s", "</td>";
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
      </table>
      
    </td>
  </tr>
  <tr> 
    <td colspan="2"> <table class="footer-bar">
        <tr> 
          <td> <iframe name="stats" src="footer.php" height="35">ed2klink</iframe> 
          </td>
          <td> <iframe name="stats" src="stats.php" height="35">connection</iframe> 
          </td>
        </tr>
      </table></td>
  </tr>
</table>
</body>
</html>
