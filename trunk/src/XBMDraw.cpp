// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "XBMDraw.h"		// Interface declarations.

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

XBMDraw::XBMDraw()
{
	m_nWidth = m_nHeight = 0;
	m_pImage = NULL;
}

XBMDraw::~XBMDraw()
{
	try
	{
		if(m_pImage)
			delete[] m_pImage;
	}
	catch(...)
	{
	}
}

bool XBMDraw::CreateImage(CString sName, int nWidth, int nHeight, BYTE bBackground)
{
	try
	{
		m_sName = sName;
		m_nWidth = nWidth;
		m_nHeight = nHeight;

		if(m_pImage)
			delete[] m_pImage;

		m_pImage = new BYTE[m_nWidth * m_nHeight];
		memset(m_pImage, bBackground, m_nWidth * m_nHeight);

		return true;
	}
	catch(...)
	{
	}
	return false;
}

bool XBMDraw::GetImage(CString &sImage)
{
	try
	{
		if(m_pImage)
		{
			int nRealWidth = m_nWidth / 8;
			if(m_nWidth % 8 != 0)
				nRealWidth++;
			long nSize = nRealWidth * m_nHeight * 6 + 1;
			BYTE *sBits = new BYTE[nSize];
			memset(sBits, 0, nSize);
			int nCount = 0;
			for(int i = 0; i < nRealWidth * m_nHeight; i++)
			{
				char sBit[20];
				sprintf(sBit, "0x%x", m_pImage[i]);
				if(i < (nRealWidth * m_nHeight - 1))
					strcat(sBit, ", ");

				for(unsigned int j = 0; j < strlen(sBit) + 1; j++)
					sBits[nCount + j] = sBit[j];

				nCount += strlen(sBit);
			}

			// wxwindows printf/format just wont work!
			//sImage.Printf("\n<script language=\"javascript\">\n%s = \'#define _width %d\\n#define _height %d\\nstatic unsigned char _bits[] = { %s }\'\n</script>\n",
			//	m_sName.GetData(), m_nWidth, m_nHeight, sBits);
			
			// hopefully this is enough
			char *tmpbuf=new char[2*nSize];			
			sprintf(tmpbuf,"\n<script language=\"javascript\">\n%s = \'#define _width %d\\n#define _height %d\\nstatic unsigned char _bits[] = { %s }\'\n</script>\n",
				unicode2char(m_sName),m_nWidth,m_nHeight,sBits);

			sImage=CString(tmpbuf);
			delete[] sBits;
			delete[] tmpbuf;

			return true;
		}
	}
	catch(...)
	{
	}
	return false;
}

bool XBMDraw::Plot(int x, int y, bool bXOR)
{
	try
	{
		if(m_pImage)
		{
			if(x >= 0 && x < m_nWidth && y >= 0 && y < m_nHeight)
			{
				int nRealWidth = m_nWidth / 8;
				if(m_nWidth % 8 != 0)
					nRealWidth++;
				int nPos = nRealWidth * (m_nHeight - y - 1) + (x / 8);
				BYTE nVal = 1 << (x % 8);
				if(m_pImage[nPos] & nVal && bXOR)
					m_pImage[nPos] &= (0xff & ~nVal);
				else
					m_pImage[nPos] |= nVal;
				
				return true;
			}
		}
	}
	catch(...)
	{
	}
	return false;
}

CString XBMDraw::GetImageTag()
{
	try
	{
		CString sRet;
		sRet.Printf(wxT("<img src=\"javascript:%s\" width=\"%d\" height=\"%d\">"), m_sName, m_nWidth, m_nHeight);
		return sRet;
	}
	catch(...)
	{
	}
	return "";
}

bool XBMDraw::Line(int x1, int y1, int x2, int y2, bool bXOR)
{
	try
	{
		if(m_pImage)
		{
			int dX = abs(x2 - x1);
			int dY = abs(y2 - y1);
			int Xincr, Yincr;
			if (x1 > x2) { Xincr=-1; } else { Xincr=1; }
			if (y1 > y2) { Yincr=-1; } else { Yincr=1; }
			
			if (dX >= dY)
			{           
				int dPr 	= dY << 1;
				int dPru 	= dPr - (dX << 1);
				int P 		= dPr - dX;

				for (; dX >= 0; dX--)
				{
					Plot(x1, y1, bXOR);
					if (P > 0)
					{ 
						x1 += Xincr;
						y1 += Yincr;
						P += dPru;
					}
					else
					{
						x1 += Xincr;
						P += dPr;
					}
				}		
			}
			else
			{
				int dPr 	= dX << 1;
				int dPru 	= dPr - (dY << 1);
				int P 		= dPr - dY;

				for (; dY >= 0; dY--)
				{
					Plot(x1, y1, bXOR);
					if (P > 0)
					{ 
						x1 += Xincr;
						y1 += Yincr;
						P += dPru;
					}
					else
					{
						y1 += Yincr;
						P += dPr;
					}
				}		
			}		
				
			return true;
		}
	}
	catch(...)
	{
	}
	return false;
}
