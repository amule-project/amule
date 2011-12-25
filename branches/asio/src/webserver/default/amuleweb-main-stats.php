<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<?php
	if ( $_SESSION["auto_refresh"] > 0 ) {
		echo "<meta http-equiv=\"refresh\" content=\"", $_SESSION["auto_refresh"], '">';
	}

	amule_load_vars("stats_graph");

?>
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
var openImg = new Image();
openImg.src = "tree-open.gif";
var closedImg = new Image();
closedImg.src = "tree-closed.gif";

function showBranch(branch){
	var objBranch = document.getElementById(branch).style;
	if(objBranch.display=="block")
		objBranch.display="none";
	else
		objBranch.display="block";
}

function swapFolder(img){
	objImg = document.getElementById(img);
	if(objImg.src.indexOf('tree-closed.gif')>-1)
		objImg.src = openImg.src;
	else
		objImg.src = closedImg.src;
}

</script>
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
    <td colspan="2">
        <table width="100%" border="0" cellspacing="0" cellpadding="0">
          <caption>
        STATISTICS 
        </caption>
          <tr> 
            <td width="24"><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td background="images/tab_top.png">&nbsp;</td>
            <td width="24"><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td width="24" background="images/tab_left.png">&nbsp;</td>
            
          <td bgcolor="#FFFFFF"><table width="100%" border="0" align="center" cellpadding="0" cellspacing="0">
              <tr valign="top"> 
                <td rowspan="7">
<iframe name="stats" src="stats_tree.php" width="100%" height="630" frameborder="0">liste</iframe></td>
                <td width="500" align="right"><img src="amule_stats_download.png" width="500" height="200" border="0"></td>
              </tr>
              <tr valign="top"> 
                <td width="500" height="15"> 
                  <div align="center">Download-Speed</div></td>
              </tr>
              <tr valign="top"> 
                <td width="500" align="right"><img src="amule_stats_upload.png" width="500" height="200" border="0" alt="" title="" /></td>
              </tr>
              <tr valign="top"> 
                <td width="500" height="15"> 
                  <div align="center">Upload-Speed</div></td>
               </tr>
              <tr valign="top"> 
                <td width="500" align="right"><img src="amule_stats_conncount.png" width="500" height="200" border="0" alt="" title="" /></td>
              </tr>
              <tr valign="top"> 
                <td width="500" height="15"> 
                  <div align="center">Connections</div></td>
              </tr>
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
