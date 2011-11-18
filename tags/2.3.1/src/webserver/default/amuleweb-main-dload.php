<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}
?><meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

<script language="JavaScript" type="text/JavaScript">
<!--
function MM_swapImgRestore() { //v3.0
  var i,x,a=document.MM_sr; for(i=0;a&&i<a.length&&(x=a[i])&&x.oSrc;i++) x.src=x.oSrc;
}

function MM_preloadImages() { //v3.0
  var d=document; if(d.images){ if(!d.MM_p) d.MM_p=new Array();
    var i,j=d.MM_p.length,a=MM_preloadImages.arguments; for(i=0; i<a.length; i++)
    if (a[i].indexOf("#")!=0){ d.MM_p[j]=new Image; d.MM_p[j++].src=a[i];}}
}

function MM_findObj(n, d) { //v4.01
  var p,i,x;  if(!d) d=document; if((p=n.indexOf("?"))>0&&parent.frames.length) {
    d=parent.frames[n.substring(p+1)].document; n=n.substring(0,p);}
  if(!(x=d[n])&&d.all) x=d.all[n]; for (i=0;!x&&i<d.forms.length;i++) x=d.forms[i][n];
  for(i=0;!x&&d.layers&&i<d.layers.length;i++) x=MM_findObj(n,d.layers[i].document);
  if(!x && d.getElementById) x=d.getElementById(n); return x;
}

function MM_swapImage() { //v3.0
  var i,j=0,x,a=MM_swapImage.arguments; document.MM_sr=new Array; for(i=0;i<(a.length-2);i+=3)
   if ((x=MM_findObj(a[i]))!=null){document.MM_sr[j++]=x; if(!x.oSrc) x.oSrc=x.src; x.src=a[i+2];}
}

function MM_nbGroup(event, grpName) { //v6.0
  var i,img,nbArr,args=MM_nbGroup.arguments;
  if (event == "init" && args.length > 2) {
    if ((img = MM_findObj(args[2])) != null && !img.MM_init) {
      img.MM_init = true; img.MM_up = args[3]; img.MM_dn = img.src;
      if ((nbArr = document[grpName]) == null) nbArr = document[grpName] = new Array();
      nbArr[nbArr.length] = img;
      for (i=4; i < args.length-1; i+=2) if ((img = MM_findObj(args[i])) != null) {
        if (!img.MM_up) img.MM_up = img.src;
        img.src = img.MM_dn = args[i+1];
        nbArr[nbArr.length] = img;
    } }
  } else if (event == "over") {
    document.MM_nbOver = nbArr = new Array();
    for (i=1; i < args.length-1; i+=3) if ((img = MM_findObj(args[i])) != null) {
      if (!img.MM_up) img.MM_up = img.src;
      img.src = (img.MM_dn && args[i+2]) ? args[i+2] : ((args[i+1])? args[i+1] : img.MM_up);
      nbArr[nbArr.length] = img;
    }
  } else if (event == "out" ) {
    for (i=0; i < document.MM_nbOver.length; i++) {
      img = document.MM_nbOver[i]; img.src = (img.MM_dn) ? img.MM_dn : img.MM_up; }
  } else if (event == "down") {
    nbArr = document[grpName];
    if (nbArr)
      for (i=0; i < nbArr.length; i++) { img=nbArr[i]; img.src = img.MM_up; img.MM_dn = 0; }
    document[grpName] = nbArr = new Array();
    for (i=2; i < args.length-1; i+=2) if ((img = MM_findObj(args[i])) != null) {
      if (!img.MM_up) img.MM_up = img.src;
      img.src = img.MM_dn = (args[i+1])? args[i+1] : img.MM_up;
      nbArr[nbArr.length] = img;
  } }
}
//-->
</script>

<link href="style.css" rel="stylesheet" type="text/css">
<style type="text/css">
<!--
caption {
	font-family: Helvetica;
	font-size: 18px;
	font-weight: bold;
	color: #003161;
}
th {
	font-family: Helvetica;
	font-size: 14px;
	font-height: 22px;
	font-weight: bold;
	color: #003161;
}
a:link {
	color: #003161;
	text-decoration: none;
}
a:active {
	color: #003161;
	text-decoration: none;
}
a:visited {
	color: #003161;
	text-decoration: none;
}
a:hover {
	color: #c0c0c0;
	text-decoration: underline;
}
td {
	font-family: Helvetica;
	font-size: 12px;
	font-weight: normal;
}
label {
	font-family: Helvetica;
	font-size: 14px;
	font-weight: bold;
}
.texte {
	font-family: Helvetica;
	font-size: 12px;
	font-weight: normal;
}
label {
font-family:"trebuchet ms",sans-serif;
font-size: 12px;
font-weight:bold
}
input {
border:1px solid #003161;
background-color:  white;
font-family:"trebuchet ms",sans-serif;
font-size: 12px;
color: #003161;
}
select, option {
background-color:  white;
font-size: 12px;
color: #003161;
}
textarea {
border:1px solid #003161;
background-color: #90B6DB;
font-family:"trebuchet ms",sans-serif;
font-size: 12px;
color: white;
}
-->
</style>
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
<body background="images/fond.gif" leftmargin="0" topmargin="0" marginwidth="0" marginheight="0" onLoad="MM_preloadImages('images/transf_1.png','images/shared_1.png','images/search_1.png','images/edkserv_1.png','images/sheserv_1.png','images/stats_1.png')">
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
            <a href="amuleweb-main-log.php">log &bull;</a> <a href="amuleweb-main-prefs.php">configuration</a>
            </td>
          <td width="10"></td>
        </tr>
      </table></td>
  </tr>
  <tr align="center" valign="top"> 
    <td colspan="2"><form action="amuleweb-main-dload.php" method="post" name="mainform">
        <table width="100%" border="0" cellpadding="0" cellspacing="0">
          <tr>
      <td align="center"><table border="0" cellpadding="0" cellspacing="0">
          <tr> 
            <td><input type="hidden" name="command"></td>
            <td><a href="javascript:formCommandSubmit('pause');" onClick="MM_nbGroup('down','group1','pause','',1)" onMouseOver="MM_nbGroup('over','pause','','',1)" onMouseOut="MM_nbGroup('out')"><img name="pause" src="images/pause.png" alt="pause" border="0" onLoad=""></a></td>
            <td><a href="javascript:formCommandSubmit('resume');" onClick="MM_nbGroup('down','group1','resume','',1)" onMouseOver="MM_nbGroup('over','resume','','',1)" onMouseOut="MM_nbGroup('out')"><img img name="resume" src="images/play.png" alt="resume" border="0" onLoad=""></a></td>
        		<td><a href="javascript:formCommandSubmit('prioup');" onClick="MM_nbGroup('down','group1','up','',1)" onMouseOver="MM_nbGroup('over','up','','',1)" onMouseOut="MM_nbGroup('out')"><img img name="prioup" src="images/up.png" alt="prioup" border="0" onLoad=""></a></td>
        		<td><a href="javascript:formCommandSubmit('priodown');" onClick="MM_nbGroup('down','group1','down','',1)" onMouseOver="MM_nbGroup('over','down','','',1)" onMouseOut="MM_nbGroup('out')"><img img name="priodown" src="images/down.png" alt="priodown" border="0" onLoad=""></a></td>
        		<td><a href="javascript:formCommandSubmit('cancel');" onClick="MM_nbGroup('down','group1','cancel','',1)" onMouseOver="MM_nbGroup('over','delete','','',1)" onMouseOut="MM_nbGroup('out')"><img img name="cancel" src="images/close.png" alt="cancel" border="0" onLoad=""></a></td>
      <td><table border="0" cellpadding="0" cellspacing="0">
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
        		echo (($s == $_SESSION["filter_status"]) ? '<option selected>' : '<option>'), $s, '</option>';
        	}
        	echo '</select>';
        	//var_dump($_SESSION["filter_cat"]);
        	echo '<select name="category" id="category">';
			$cats = amule_get_categories();
			foreach($cats as $c) {
				echo (($c == $_SESSION["filter_cat"]) ? '<option selected>' : '<option>'), $c, '</option>';
			}
			echo '</select>';
        ?>
                  </td>
                  			<td><a href="javascript:formCommandSubmit('filter');" onClick="MM_nbGroup('down','group1','resume','',1)" onMouseOver="MM_nbGroup('over','resume','','',1)" onMouseOut="MM_nbGroup('out')"><img src="images/filter.png" border="0" alt="Apply" name="resume" border="0" onload=""></a></td>
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
      <td height="10" align="center"> </td>
    </tr>
    <tr> 
      <td colspan="2"><table width="100%" border="0" cellspacing="0" cellpadding="0"> <caption>
          DOWNLOAD 
          </caption>
  <tr>
    <td width="24"><img src="images/tab_top_left.png" width="24" height="24"></td>
    <td background="images/tab_top.png">&nbsp;</td>
    <td width="24"><img src="images/tab_top_right.png" width="24" height="24"></td>
  </tr>
  <tr>
    <td width="24" background="images/tab_left.png">&nbsp;</td>
    <td bgcolor="#FFFFFF"><table width="100%" border="0" align="center" cellpadding="0" cellspacing="0">
                     
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
          </tr><tr><td colspan="9" height="1" bgcolor="#000000"></td></tr>
          <?php
		function CastToXBytes($size)
		{
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
				case "scrcount": $result = $a->src_count > $b->src_count; break;
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
		
		$downloads = amule_load_vars("downloads");

		$sort_order = $HTTP_GET_VARS["sort"];

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
	
				echo "<td class='texte' height='22'>", '<input type="checkbox" name="', $file->hash, '" >', "</td>";
	
				echo "<td class='texte' height='22'>", $file->short_name, "</td>";
				
				echo "<td class='texte' height='22' align='center'>", CastToXBytes($file->size), "</td>";

				echo "<td class='texte' height='22' align='center'>", CastToXBytes($file->size_done), "&nbsp;(",
					((float)$file->size_done*100)/((float)$file->size), "%)</td>";

				echo "<td class='texte' height='22' align='center'>", ($file->speed > 0) ? (CastToXBytes($file->speed) . "/s") : "-", "</td>";

				echo "<td class='texte' height='22' align='center' align='center'>", $file->progress, "</td>";
	
				echo "<td class='texte' height='22' align='center'>";
				if ( $file->src_count_not_curr != 0 ) {
					echo $file->src_count - $file->src_count_not_curr, " / ";
				}
				echo $file->src_count, " ( ", $file->src_count_xfer, " ) ";
				if ( $file->src_count_a4af != 0 ) {
					echo "+ ", $file->src_count_a4af;
				}
				echo "</td>";
	
				echo "<td class='texte' height='22' align='center'>", StatusString($file), "</td>";
				
				echo "<td class='texte' height='22' align='center'>", PrioString($file), "</td>";
				
				print "</tr><tr><td colspan='9' height='1' bgcolor='#c0c0c0'></td></tr>";
			}
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
</table></td>
    </tr>
  </table>
</form>
      <table width="100%" border="0" cellspacing="0" cellpadding="0"><caption>
        UPLOAD 
        </caption>
        <tr> 
          <td width="24"><img src="images/tab_top_left.png" width="24" height="24"></td>
          <td background="images/tab_top.png">&nbsp;</td>
          <td width="24"><img src="images/tab_top_right.png" width="24" height="24"></td>
        </tr>
        <tr> 
          <td width="24" background="images/tab_left.png">&nbsp;</td>
          <td bgcolor="#FFFFFF"><table width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="doad-table">
              
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
        </tr><tr><td colspan="9" height="1" bgcolor="#000000"></td></tr>
        <?php
			function CastToXBytes($size)
			{
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
			$uploads = amule_load_vars("uploads");
			foreach ($uploads as $file) {
				echo "<tr>";
	
				echo "<td class='texte' height='22' align='center'>", "</td>";
				
				echo "<td class='texte' height='22'>", $file->short_name, "</td>";

				echo "<td class='texte' height='22' align='center'>", $file->user_name, "</td>";
	
				echo "<td class='texte' height='22' align='center'>", CastToXBytes($file->xfer_up), "</td>";
				echo "<td class='texte' height='22' align='center'>", CastToXBytes($file->xfer_down), "</td>";
				echo "<td class='texte' height='22' align='center'>", "</td>";
				echo "<td class='texte' height='22' align='center'>", "</td>";
				echo "<td class='texte' height='22' align='center'>", ($file->xfer_speed > 0) ? (CastToXBytes($file->xfer_speed) . "/s") : "-", "</td>";
				echo "<td class='texte' height='22' align='center'>", "</td>";
				echo "</tr><tr><td colspan='9' height='1' bgcolor='#c0c0c0'></td></tr>";
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
      </table>
      
    </td>
  </tr>
  <tr valign="bottom"> 
    <td height="25" colspan="2"> <table width="100%" height="40" border="0" cellpadding="0" cellspacing="0">
        <tr align="center" valign="middle"> 
          <td width="50%"> <iframe name="stats" src="footer.php" height="35" width="100%" scrolling="no" frameborder="0">ed2klink</iframe> 
          </td>
          <td width="50%"> <iframe name="stats" src="stats.php" height="35" width="100%" scrolling="no" frameborder="0">connection</iframe> 
          </td>
        </tr>
      </table></td>
  </tr>
</table>
</body>
</html>
