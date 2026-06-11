<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title><script language="JavaScript" type="text/javascript">

function login_init()
{
	document.login.pass.focus();
}

</script>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link href="style.css" rel="stylesheet" type="text/css">
</head>

<body  onload="login_init();" background="images/fond.gif" leftmargin="0" topmargin="0" marginwidth="0" marginheight="0">
<table width="100%" height="180" border="0" cellpadding="0" cellspacing="0" valign="middle">
  <tr>
    <td align="center" valign="middle"> 
      <table width="70%" height="90%" border="0" align="center" cellpadding="0" cellspacing="1" bgcolor="#000000">
        <tr> 
          <td><table width="100%" height="100%" border="0" align="center" cellpadding="0" cellspacing="0" bgcolor="#FFFFFF">
              <tr valign="top"> 
               <th width="366" height="180"><img src="images/loginlogo.jpg" width="366" height="180" border="0"></th>
                <th width="100% "height="180" align="right" valign="middle" background="images/loginfond_haut.png"> 
                  <form action="" method="post" name="login">
                    Enter password : 
                    <input name="pass" size="20" value="" type="password">
                    &nbsp; 
                    <input name="submit" type="submit" value="Submit">
                    &nbsp;&nbsp;
<?php
	if ( $_SESSION["login_error"] != "" ) {
		echo "<br><span style=\"color: #c00000; font-weight: bold;\">", $_SESSION["login_error"], "</span>&nbsp;&nbsp;";
	}
?>
                    </form></th>
              </tr>
            </table></td>
        </tr>
      </table>
    </td>
  </tr>
</table>
</body>
</html>
