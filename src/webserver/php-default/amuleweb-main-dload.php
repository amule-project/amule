<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule download page</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
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
<script language="JavaScript" type="text/JavaScript">
<!--
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
</head>
<script language="JavaScript" type="text/JavaScript">
function formCommandSubmit(command)
{
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

</script>

<body>
<form name="mainform" action="amuleweb-main-dload.php" method="post">
<table width="100%"  border="1" bgcolor="#0099CC">
  <tr>
    <td width="94" height="20"><input type="hidden" name="command"></td>
    <td width="363"><table border="0" cellpadding="0" cellspacing="0">
      <tr>
        <td><a href="javascript:formCommandSubmit('pause');" target="mainFrame" onClick="MM_nbGroup('down','group1','pause','',1)" onMouseOver="MM_nbGroup('over','pause','','',1)" onMouseOut="MM_nbGroup('out')"><img name="pause" src="pause.jpeg" border="0" alt="" onLoad=""></a></td>
        <td><a href="javascript:formCommandSubmit('resume');" target="mainFrame" onClick="MM_nbGroup('down','group1','resume','',1)" onMouseOver="MM_nbGroup('over','resume','','',1)" onMouseOut="MM_nbGroup('out')"><img name="resume" src="resume.jpeg" border="0" alt="" onLoad=""></a></td>
        <td><a href="javascript:formCommandSubmit('prioup');" target="mainFrame" onClick="MM_nbGroup('down','group1','up','',1)" onMouseOver="MM_nbGroup('over','up','','',1)" onMouseOut="MM_nbGroup('out')"><img name="up" src="up.jpeg" border="0" alt="" onLoad=""></a></td>
        <td><a href="javascript:formCommandSubmit('priodown');" target="mainFrame" onClick="MM_nbGroup('down','group1','down','',1)" onMouseOver="MM_nbGroup('over','down','','',1)" onMouseOut="MM_nbGroup('out')"><img src="down.jpeg" alt="" name="down" width="50" height="20" border="0" onload=""></a></td>
        <td><a href="javascript:formCommandSubmit('delete');" target="mainFrame" onClick="MM_nbGroup('down','group1','delete','',1)" onMouseOver="MM_nbGroup('over','delete','','',1)" onMouseOut="MM_nbGroup('out')"><img src="delete.jpeg" alt="" name="delete" width="50" height="20" border="0" onload=""></a></td>
      </tr>
    </table></td>
    <td width="264"><table border="0" cellpadding="0" cellspacing="0">
      <tr>
        <td>
        <?php
        	$all_status = array("All", "Waiting", "Paused", "Downloading");
        	
 			if ( $HTTP_GET_VARS["command"] == "filter") $_SESSION["filter_status"] = $HTTP_GET_VARS["status"];
        	if ( $_SESSION["filter_status"] == '') $_SESSION["filter_status"] = 'All';

        	echo '<select name ="status"';
        	foreach ($all_status as $s) {
        		echo (($s == $_SESSION["filter_status"]) ? '<option selected>' : '<option>'), $s, '</option>';
        	}
        	echo '</select>';
        	
        ?>
        <!--
        <select name="status">
          <option selected>All</option>
          <option>Waiting</option>
          <option>Paused</option>
          <option>Downloading</option>
        </select>
        --!>
        </td>
        <td><a href="javascript:formCommandSubmit('filter');" target="mainFrame" onClick="MM_nbGroup('down','group1','resume','',1)" onMouseOver="MM_nbGroup('over','resume','','',1)" onMouseOut="MM_nbGroup('out')"><img src="apply.jpeg" alt="Apply" name="resume" width="50" height="20" border="0" onload=""></a></td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
    </table></td>
  </tr>
  <tr>
    <td colspan="3"><table width="100%"  border="0" cellpadding="2" cellspacing="2">
      <tr>
        <th width="22" scope="col">&nbsp;</th>
        <th width="230" scope="col"><div align="left"><a href="amuleweb-main-dload.php?sort=name" target="mainFrame">Filename</a></div></th>
        <th width="227" scope="col"><div align="left">
          <div align="left">Progress</div></th>
        <th width="231" scope="col"><div align="left">
          <div align="left"><a href="amuleweb-main-dload.php?sort=size" target="mainFrame">Size</a></div></th>
        <th width="44" scope="col"><div align="left"><a href="amuleweb-main-dload.php?sort=status" target="mainFrame">Status</a></div></th>
        <th width="88" nowrap scope="col"><div align="left"><a href="amuleweb-main-dload.php?sort=speed" target="mainFrame">Speed</a></div></th>
        <th width="8" scope="col">&nbsp;</th>
        <th width="12" scope="col">&nbsp;</th>
        </tr>
      <tr>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td nowrap scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        </tr>
	  
	  <?php
		function CastToXBytes($size)
		{
			if ( $size < 1024 ) {
				$result = $size . " bytes";
			} elseif ( $size < 1048576 ) {
				$result = ($size / 1024.0) . "KB";
			} elseif ( $size < 1073741824 ) {
				$result = ($size / 1048576.0) . "MB";
			} elseif ( $size < 1099511627776 ) {
				$result = ($size / 1073741824.0) . "GB";
			} else {
				$result = "Too big";
			}
			return $result;
		}

		// FIXME: replace with switch when supported by interpreter
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

		//
		// declare it here, before any function reffered it in "global"
		//
		$sort_order;$sort_reverse;

		function my_cmp($a, $b)
		{
			global $sort_order, $sort_reverse;
			if ( $sort_order == "size" ) {
				$result = $a->size > $b->size;
			} elseif ( $sort_order == "name" ) {
				$result = $a->name > $b->name;
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
		if ( $HTTP_GET_VARS["command"] != "") {
			//amule_do_download_cmd($HTTP_GET_VARS["command"]);
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
			//var_dump($HTTP_GET_VARS["command"]);
			if ( $HTTP_GET_VARS["command"] == "filter") {
				$_SESSION["filter_status"] = $HTTP_GET_VARS["status"];
			//var_dump($HTTP_GET_VARS["command"]);
			}
		}
		//var_dump($_SESSION["filter_status"]);
		if ( $_SESSION["filter_status"] == "") $_SESSION["filter_status"] = "All";
		
		$downloads = amule_load_vars("downloads");

		$sort_order = $HTTP_GET_VARS["sort"];

		if ( $sort_order == "" ) {
			$sort_order = $_SESSION["download_sort"];
		} else {
			if ( $_SESSION["sort_reverse"] == "" ) {
				$_SESSION["sort_reverse"] = 0;
			} else {
				$_SESSION["sort_reverse"] = !$_SESSION["sort_reverse"];
			}
		}
		var_dump($_SESSION);
		$sort_reverse = $_SESSION["sort_reverse"];
		if ( $sort_order != "" ) {
			$_SESSION["download_sort"] = $sort_order;
			usort($downloads, "my_cmp");
		}

		foreach ($downloads as $file) {
			if ( ($_SESSION["filter_status"] == "All") or ( $_SESSION["filter_status"] == StatusString($file) ) ) {
			print "<tr>";

			echo "<td>", '<input type="checkbox" name="', $file->hash, '" >', "</td>";

			echo "<td nowrap>", $file->name, "</td>";

			echo "<td>", $file->progress, "</td>";
			
			echo "<td>", CastToXBytes($file->size), "</td>";

			echo "<td>", StatusString($file), "</td>";

			echo "<td>", ($file->speed > 0) ? (CastToXBytes($file->speed) . "/s") : "-", "</td>";
			print "</tr>";
			}
		}
	  ?>
    </table></td>
  </tr>
</table>
</form>
</body>
</html>
