<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

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
//-->
</script>
<link href="style.css" rel="stylesheet" type="text/css"><style type="text/css">
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
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

var initvals = new Object;

<?php
	// apply new options before proceeding
	//var_dump($HTTP_GET_VARS);
	if ( ($HTTP_GET_VARS["Submit"] == "Apply") && ($_SESSION["guest_login"] == 0) ) {
		$file_opts = array("check_free_space", "extract_metadata", 
			"ich_en","aich_trust", "preview_prio","save_sources", "resume_same_cat",
			"min_free_space", "new_files_paused", "alloc_full", "alloc_full_chunks",
			"new_files_auto_dl_prio", "new_files_auto_ul_prio"
		);
		$conn_opts = array("max_line_up_cap","max_up_limit",
			"max_line_down_cap","max_down_limit", "slot_alloc", 
			"tcp_port","udp_dis","max_file_src","max_conn_total","autoconn_en");
		$webserver_opts = array("use_gzip", "autorefresh_time");
		
		$all_opts;
		foreach ($conn_opts as $i) {
			$curr_value = $HTTP_GET_VARS[$i];
			if ( $curr_value == "on") $curr_value = 1;
			if ( $curr_value == "") $curr_value = 0;
			
			$all_opts["connection"][$i] = $curr_value;
		}
		foreach ($file_opts as $i) {
			$curr_value = $HTTP_GET_VARS[$i];
			if ( $curr_value == "on") $curr_value = 1;
			if ( $curr_value == "") $curr_value = 0;
			
			$all_opts["files"][$i] = $curr_value;
		}
		foreach ($webserver_opts as $i) {
			$curr_value = $HTTP_GET_VARS[$i];
			if ( $curr_value == "on") $curr_value = 1;
			if ( $curr_value == "") $curr_value = 0;
			
			$all_opts["webserver"][$i] = $curr_value;
		}
		//var_dump($all_opts);
		amule_set_options($all_opts);
	}
	$opts = amule_get_options();
	//var_dump($opts);
	$opt_groups = array("connection", "files", "webserver");
	//var_dump($opt_groups);
	foreach ($opt_groups as $group) {
		$curr_opts = $opts[$group];
		//var_dump($curr_opts);
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
		"min_free_space",
		"autorefresh_time"
		)
	for(i = 0; i < str_param_names.length; i++) {
		frm[str_param_names[i]].value = initvals[str_param_names[i]];
	}
	var check_param_names = new Array(
		"autoconn_en", "reconn_en", "udp_dis", "new_files_paused",
		"aich_trust", "alloc_full", "alloc_full_chunks",
		"check_free_space", "extract_metadata", "ich_en",
		"new_files_auto_dl_prio", "new_files_auto_ul_prio",
		"use_gzip"
		)
	for(i = 0; i < check_param_names.length; i++) {
		frm[check_param_names[i]].checked = initvals[check_param_names[i]] == "1" ? true : false;
	}
}

</script>
<body background="images/fond.gif" leftmargin="0" topmargin="0" marginwidth="0" marginheight="0" onLoad="MM_preloadImages('images/transf_1.png','images/shared_1.png','images/search_1.png','images/edkserv_1.png','images/sheserv_1.png','images/stats_1.png');init_data();">
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
    <td colspan="2">
        <table width="100%" border="0" cellspacing="0" cellpadding="0">
          <caption>PREFERENCES</caption>
          <tr> 
            <td width="24"><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td background="images/tab_top.png">&nbsp;</td>
            <td width="24"><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td width="24" background="images/tab_left.png">&nbsp;</td>
            
      <td bgcolor="#FFFFFF"><form name="mainform" action="amuleweb-main-prefs.php" method="post">
              <table border="0" align="center" cellpadding="0" cellspacing="6">
               
    <tr align="center" valign="top"> 
      <td> 
        <table width="350" border="0" align="center" cellpadding="0" cellspacing="0" >
          <tr> 
            <td width="22">&nbsp;</td>
            <th>Webserver</th>
            <td width="63">&nbsp;</td>
          </tr>
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Page refresh interval </td>
            <td width="63" height="25">
<input name="autorefresh_time" type="text" id="autorefresh_time7" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">
<input name="use_gzip" type="checkbox" id="use_gzip5"></td>
            <td height="25"> Use gzip compression </td>
            <td width="63" height="25">&nbsp;</td>
          </tr>
        </table></td>
      <td width="50%"> 
        <table width="350" border="0" align="center" cellpadding="0" cellspacing="0" >
          <tr> 
            <td width="22">&nbsp;</td>
            <th>Line capacity (for statistics only)</th>
            <td width="63">&nbsp;</td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Max download rate </td>
            <td width="63" height="25">
<input name="max_line_down_cap" type="text" id="max_line_down_cap6" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Max upload rate </td>
            <td width="63" height="25">
<input name="max_line_up_cap" type="text" id="max_line_up_cap7" size="4"></td>
          </tr>
        </table></td>
    </tr>
    <tr align="center" valign="top"> 
      <td> 
        <table width="350" border="0" align="center" cellpadding="0" cellspacing="0" >
          <tr> 
            <td width="22" height="19">&nbsp;</td>
            <th>Bandwidth limits</th>
            <td width="63">&nbsp;</td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Max download rate </td>
            <td width="63" height="25">
<input name="max_down_limit" type="text" id="max_down_limit6" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Max upload rate </td>
            <td width="63" height="25">
<input name="max_up_limit" type="text" id="max_up_limit6" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Slot allocation </td>
            <td width="63" height="25">
<input name="slot_alloc" type="text" id="slot_alloc6" size="4"></td>
          </tr>
        </table></td>
      <td width="50%" rowspan="3"> 
        <table width="350" border="0" align="center" cellpadding="0" cellspacing="0" >
          <tr> 
            <td width="22">&nbsp;</td>
            <th> File settings </th>
            <td width="63">&nbsp;</td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">&nbsp; </td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="check_free_space" type="checkbox" id="check_free_space5"></td>
            <td height="25"> Check free space =&gt; Minimum free space (Mb) </td>
            <td width="63" height="25"> 
              <input name="min_free_space" type="text" id="min_free_space4" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="new_files_auto_dl_prio" type="checkbox" id="new_files_auto_dl_prio4"></td>
            <td height="25"> Added download files have auto priority</td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="new_files_auto_ul_prio" type="checkbox" id="new_files_auto_ul_prio4"></td>
            <td height="25"> New shared files have auto priority</td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="ich_en" type="checkbox" id="ich_en5"></td>
            <td height="25"> I.C.H. active</td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="aich_trust" type="checkbox" id="aich_trust4"></td>
            <td height="25"> AICH trusts every hash (not recommended)</td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="alloc_full_chunks" type="checkbox" id="alloc_full_chunks4"></td>
            <td height="25"> Alloc full chunks of .part files</td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="alloc_full" type="checkbox" id="alloc_full4"></td>
            <td height="25"> Alloc full disk space for .part files</td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="new_files_paused" type="checkbox" id="new_files_paused4"></td>
            <td height="25"> Add files to download queue in pause mode</td>
            <td width="63" height="25"></td>
          </tr>
          <tr> 
            <td width="22" height="25"> 
              <input name="extract_metadata" type="checkbox" id="extract_metadata4"></td>
            <td height="25"> Extract metadata tags </td>
            <td width="63" height="25"></td>
          </tr>
        </table></td>
    </tr>
    <tr align="center" valign="top"> 
      <td> 
        <table width="350" border="0" align="center" cellpadding="0" cellspacing="0" >
          <tr> 
            <td width="22">&nbsp;</td>
            <th>Connection settings</th>
            <td width="63">&nbsp;</td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Max total connections (total) </td>
            <td width="63" height="25">
<input name="max_conn_total" type="text" id="max_conn_total8" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">Max sources per file </td>
            <td width="63" height="25">
<input name="max_file_src" type="text" id="max_file_src7" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">
<input name="autoconn_en" type="checkbox" id="autoconn_en6"></td>
            <td height="25"> Autoconnect at startup </td>
            <td width="63" height="25">&nbsp;</td>
          </tr>
          <tr> 
            <td width="22" height="25">
<input name="reconn_en" type="checkbox" id="reconn_en6"></td>
            <td height="25"> Reconnect when connection lost </td>
            <td width="63" height="25">&nbsp;</td>
          </tr>
        </table></td>
    </tr>
    <tr align="center" valign="top"> 
      <td> 
        <table width="350" border="0" align="center" cellpadding="0" cellspacing="0" >
          <tr> 
            <td width="22">&nbsp;</td>
            <th>Network settings</th>
            <td width="63">&nbsp;</td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">TCP port </td>
            <td width="63" height="25">
<input name="tcp_port" type="text" id="tcp_port6" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">&nbsp;</td>
            <td height="25">UDP port </td>
            <td width="63" height="25">
<input name="udp_port" type="text" id="udp_port6" size="4"></td>
          </tr>
          <tr> 
            <td width="22" height="25">
<input name="udp_dis" type="checkbox" id="udp_dis5"></td>
            <td height="25"> Disable UDP connections </td>
            <td width="63" height="25">&nbsp;</td>
          </tr>
        </table></td>
    </tr>
    <tr align="center"> 
      <td colspan="2"> 
        <?php
			if ($_SESSION["guest_login"] == 0) {
				echo '<input type="submit" name="Submit" value="Apply">';
			} else {
				echo "<b>&nbsp;You can not change options - logged in as guest</b>";
			}
		?>
        <input name="command" type="hidden" id="command"> </td>
    </tr>
  </table>
  </form></td>
            <td width="24" background="images/tab_right.png">&nbsp;</td>
          </tr>
          <tr> 
            <td width="24"><img src="images/tab_bottom_left.png" width="24" height="24"></td>
            <td background="images/tab_bottom.png">&nbsp;</td>
            <td width="24"><img src="images/tab_bottom_right.png" width="24" height="24"></td>
          </tr>
        </table></td>
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
