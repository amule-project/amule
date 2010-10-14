<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title><script language="JavaScript" type="text/javascript">

function breakout_of_frame()
{
  // see http://www.thesitewizard.com/archive/framebreak.shtml
  // for an explanation of this script and how to use it on your
  // own website
  if (top.location != location) {
    top.location.href = document.location.href ;
  }
}

function login_init()
{
	breakout_of_frame();
	document.login.pass.focus();
}

</script>
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

<body  onload="login_init();" background="images/fond.gif" leftmargin="0" topmargin="0" marginwidth="0" marginheight="0" valign="middle">
<table width="100%" height="180" border="0" cellpadding="0" cellspacing="0" valign="middle">
  <tr>
    <td align="center" valign="middle"> 
      <table width="70%" height="90%" border="0" align="center" cellpadding="0" cellspacing="1" bgcolor="#000000">
        <tr> 
          <td><table width="100%" height="100%" border="0" align="center" cellpadding="0" cellspacing="0" bgcolor="#FFFFFF">
              <tr valign="top"> 
               <th width="366" height="180"><img src="images/loginlogo.jpg" width="366" height="180" border="0"></a></th>
                <th width="100% "height="180" align="right" valign="middle" background="images/loginfond_haut.png"> 
                  <form action="" method="post" name="login">
                    Enter password : 
                    <input name="pass" size="20" value="" type="password">
                    &nbsp; 
                    <input name="submit" type="submit" value="Submit">
                    &nbsp;&nbsp; </form></th>
              </tr>
            </table></td>
        </tr>
      </table>
    </td>
  </tr>
</table>
</body>
</html>
