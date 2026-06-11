<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

<script type="text/javascript" src="common.js"></script>
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
<body background="images/fond.gif" leftmargin="0" topmargin="0" marginwidth="0" marginheight="0" onLoad="MM_preloadImages('images/transf_1.png','images/shared_1.png','images/search_1.png','images/edkserv_1.png','images/sheserv_1.png','images/stats_1.png');">
<table width="100%" height="100%" border="0" cellpadding="0" cellspacing="0">
  <tr valign="top"> 
    <td width="143" height="64"><img src="images/logo.png" width="143" height="64"></td>
    <td width="100%" height="64" align="right" background="images/fond_haut.png"> <table border="0" cellspacing="0" cellpadding="0">
        <tr> 
          <td><a href="amuleweb-main-dload.php" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('transfert','','images/transf_1.png',1)"><img src="images/transf_0.png" alt="transfert" name="transfert" width="52" height="50" border="0"></a></td>
          <td><a href="amuleweb-main-shared.php" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('shared','','images/shared_1.png',1)"><img src="images/shared_0.png" alt="shared" name="shared" width="52" height="50" border="0"></a></td>
          <td><a href="amuleweb-main-search.php" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('search','','images/search_1.png',1)"><img src="images/search_0.png" alt="search" name="search" width="52" height="50" border="0"></a></td>
          <td><a href="amuleweb-main-servers.php" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('edkserver','','images/edkserv_1.png',1)"><img src="images/edkserv_0.png" alt="edkserver" name="edkserver" width="52" height="50" border="0"></a></td>
          <td><a href="amuleweb-main-kad.php" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('sheserv','','images/sheserv_1.png',1)"><img src="images/sheserv_0.png" alt="sheserv" name="sheserv" width="52" height="50" border="0"></a></td>
          <td><a href="amuleweb-main-stats.php" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('statistiques','','images/stats_1.png',1)"><img src="images/stats_0.png" alt="statistiques" name="statistiques" width="52" height="50" border="0"></a></td>
          <td><img src="images/col.png"></td>
          <td width="10"></td>
          <td width="190" align="right" class="texteinv"><a href="login.php">exit</a><br> 
            <a href="amuleweb-main-log.php">log &bull;</a> <a href="amuleweb-main-prefs.php">configuration</a></td>
          <td width="10"></td>
        </tr>
      </table></td>
  </tr>
  <tr align="center" valign="top"> 
    <td colspan="2"><form name="mainform" action="amuleweb-main-shared.php" method="post">
              <table border="0" align="center" cellpadding="0" cellspacing="0">
                <tr>
                  <td><input type="hidden" name="command"></td>
                  
            <td><a href="javascript:formCommandSubmit('reload');" onClick="MM_nbGroup('down','group1','reload','',1)" onMouseOver="MM_nbGroup('over','reload','','',1)" onMouseOut="MM_nbGroup('out')"><img src="images/refresh.png" alt="Reload shared files" name="reload" border="0" onload=""></a></td>
				  <td><a href="javascript:formCommandSubmit('prioup');" onClick="MM_nbGroup('down','group1','up','',1)" onMouseOver="MM_nbGroup('over','up','','',1)" onMouseOut="MM_nbGroup('out')"><img name="up" src="images/up.png" border="0" alt="Raise priority" onLoad=""></a></td>
                  
            <td><a href="javascript:formCommandSubmit('priodown');" onClick="MM_nbGroup('down','group1','down','',1)" onMouseOver="MM_nbGroup('over','down','','',1)" onMouseOut="MM_nbGroup('out')"><img src="images/down.png" alt="Lower priority" name="down" border="0" onload=""></a></td>
                  <td><select name="select">
                      <option selected>Select prio</option>
                      <option>Low</option>
                      <option>Normal</option>
                      <option>High</option>
                    </select> </td>
                  
            <td><a href="javascript:formCommandSubmit('setprio');" onClick="MM_nbGroup('down','group1','resume','',1)" onMouseOver="MM_nbGroup('over','resume','','',1)" onMouseOut="MM_nbGroup('out')"><img src="images/ok.png" alt="Set priority" name="resume" border="0" onload=""></a></td>
              
                  <td> 
                    <?php
		 	if ($_SESSION["guest_login"] != 0) {
				echo "<b>&nbsp;You logged in as guest - commands are disabled</b>";
			}
		 ?>
                  </td>
                </tr>
              </table>
        <table width="100%" border="0" cellspacing="0" cellpadding="0">
          <caption>
        SHARED FILES 
        </caption>
          <tr> 
            <td width="24"><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td background="images/tab_top.png">&nbsp;</td>
            <td width="24"><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td width="24" background="images/tab_left.png">&nbsp;</td>
            
          <td bgcolor="#FFFFFF">
              <table width="100%"  border="0" align="center" cellpadding="0" cellspacing="0">
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
                </tr><tr><td colspan="9" height="1" bgcolor="#000000"></td></tr>
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

		$sort_order = $HTTP_GET_VARS["sort"];

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
			echo "<td class='texte' align='center'>", CastToXBytes($file->xfer), " (", CastToXBytes($file->xfer_all),")</td>";

			echo "<td class='texte' align='center'>", $file->req, " (", $file->req_all, ")</td>";
			echo "<td class='texte' align='center'>", $file->accept, " (", $file->accept_all, ")</td>";
			
			echo "<td class='texte' align='center'>", CastToXBytes($file->size), "</td>";

			echo "<td class='texte' align='center'>", PrioString($file), "</td>";;

			print "</tr><tr><td colspan='9' height='1' bgcolor='#c0c0c0'></td></tr>";
		}
	  ?>
              </table></td>
            <td width="24" background="images/tab_right.png">&nbsp;</td>
          </tr>
          <tr> 
            <td width="24"><img src="images/tab_bottom_left.png" width="24" height="24"></td>
            <td background="images/tab_bottom.png">&nbsp;</td>
            <td width="24"><img src="images/tab_bottom_right.png" width="24" height="24"></td>
          </tr>
        </table></form></td>
  </tr>
  <tr valign="bottom"> 
    <td height="25" colspan="2"> <table width="100%" height="40" border="0" cellpadding="0" cellspacing="0">
        <tr align="center" valign="middle"> 
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
