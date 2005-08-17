<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule download page</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
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
<form name="mainform" action="amuleweb-main-search.php" method="post">
<table width="100%"  border="0" bgcolor="#0099CC">
  <tr>
    <td width="100%"><table width="100%"  border="0" cellpadding="2" cellspacing="2">
      <tr>
        <td colspan="5" scope="col"><table width="100%"  border="1" align="left" cellpadding="0" cellspacing="0">
          <tr>
            <td width="625" bgcolor="#0099CC">                  <input name="searchval" type="text" id="searchval" size="75">
              <input type="submit" name="Submit" value="Search">              <div align="left">
              </div></td>
            <td width="278" bgcolor="#0099CC"><input name="Download" type="submit" id="Download" value="Download">
              <select name="select">
                <option>Any category</option>
              </select></td>
          </tr>
          <tr>
            <td colspan="2" bgcolor="#0099CC"><div align="left">
              <label>Min size</label>
            &nbsp;
                <input name="minsize" type="text" id="minsize" size="5">
                <select name="minsizeu" id="minsizeu">
                    <option>Byte</option>
                    <option>KByte</option>
                    <option selected>MByte</option>
                    <option>GByte</option>
                </select>              
                <label>Max size</label>
&nbsp;
    <input name="maxsize" type="text" id="maxsize" size="5">
    <select name="maxsizeu" id="maxsizeu">
          <option>Byte</option>
          <option>KByte</option>
          <option selected>MByte</option>
          <option>GByte</option>
    </select>  
    <label>Availability</label>
    <input name="avail" type="text" id="avail" size="6">
</div></td>
            </tr>
        </table></td>
        </tr>
      <tr>
        <th width="22" scope="col">&nbsp;</th>
        <th width="349" nowrap scope="col"><div align="left"><a href="amuleweb-main-shared.php?sort=name" target="mainFrame">Filename</a></div></th>
        <th width="99" nowrap scope="col"><div align="left"><a href="amuleweb-main-shared.php?sort=size" target="mainFrame">Size</a></div></th>
        <th width="96" nowrap scope="col"><a href="amuleweb-main-shared.php?sort=prio" target="mainFrame">Sources</a></th>
        <th width="222" nowrap scope="col">&nbsp;</th>
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

		$search = amule_load_vars("searchresult");
		foreach ($search as $file) {
			print "<tr>";

			echo "<td>", '<input type="checkbox" name="', $file->hash, '" >', "</td>";

			echo "<td nowrap>", $file->short_name, "</td>";
			
			echo "<td>", CastToXBytes($file->size), "</td>";

			echo "<td>", $file->sources, "</td>";

			print "</tr>";
		}

	  ?>

    </table></td>
  </tr>
</table>
</form>
</body>
</html>
