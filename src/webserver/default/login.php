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

<body class="main" onload="login_init();">
<table width="100%" height="180" cellpadding="0" cellspacing="0">
  <tr>
    <td class="al-center va-middle">
      <table width="70%" height="90%" class="center-table bg-black" cellpadding="0" cellspacing="1">
        <tr>
          <td><table width="100%" height="100%" class="bg-white" cellpadding="0" cellspacing="0">
              <tr class="va-top">
               <th width="366" class="h180"><img src="images/loginlogo.jpg" width="366" height="180"></th>
                <th width="100%" class="login-banner">
                  <form action="" method="post" name="login">
                    Enter password : 
                    <input name="pass" size="20" value="" type="password">
                    &nbsp; 
                    <input name="submit" type="submit" value="Submit">
                    &nbsp;&nbsp;
<?php
	if ( $_SESSION["login_error"] != "" ) {
		echo "<br><span class=\"login-error\">", $_SESSION["login_error"], "</span>&nbsp;&nbsp;";
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
