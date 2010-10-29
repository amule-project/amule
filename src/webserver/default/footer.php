<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule-frame-bottom</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
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
</head>
<script language="JavaScript" type="text/JavaScript">

function refreshFrames()
{
	location = "amuleweb-main-dload.php"
	location.reload();
}

</script>

<body>

<table width="100%" border="0" align="center" cellpadding="0" cellspacing="0">
  <tr>
    <td width="50%" align="center" valign="bottom"> 
      <form name="formlink" method="post" action="footer.php">
        <input name="ed2klink" type="text" id="ed2klink" size="50">
        <select name="selectcat" id="selectcat">
        <?php
		$cats = amule_get_categories();
		if ( $HTTP_GET_VARS["Submit"] != "" ) {
			$link = $HTTP_GET_VARS["ed2klink"];
			$target_cat = $HTTP_GET_VARS["selectcat"];
			$target_cat_idx = 0;

			foreach($cats as $i => $c) {
				if ( $target_cat == $c) $target_cat_idx = $i;
			}

			if ( strlen($link) > 0 ) {
				$links = split("ed2k://", $link);
				foreach($links as $linkn) {
				    amule_do_ed2k_download_cmd("ed2k://" . $linkn, $target_cat_idx);
				}
			}
		}

		foreach($cats as $c) {
			echo  '<option>', $c, '</option>';
		}
	?>
        </select>
        <input type="submit" name="Submit" value="Download link" onClick="refreshFrames()">
      </form></td>
</table>
</body>
</html>
