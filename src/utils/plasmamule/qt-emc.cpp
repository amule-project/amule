//
// This file is part of the aMule Project.
//
// Copyright (c) 2010-2011 Werner Mahr (Vollstrecker) <amule@vollstreckernet.de>
//
// Any parts of this program contributed by third-party developers are copyrighted
// by their respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "qt-emc.h"

qtEmc::qtEmc(const QString &filename)
{

	QStringList files = filename.split("file://");

	for (QStringList::const_iterator constFilesIterator = files.constBegin(); constFilesIterator != files.constEnd(); constFilesIterator++)
	{
		if (!QString(*constFilesIterator).remove("\n").trimmed().isEmpty())
		{
			QFile collection(QString(*constFilesIterator).trimmed());

			if (collection.open (QIODevice::ReadOnly))
			{
				valid = readBinary(collection);
			} else {
				valid = FALSE;
				errorCode = BadFileFormat;
			}
	
			collection.close();
		}
	}
}

const int qtEmc::getError()
{
	return errorCode;
}

const QString qtEmc::getErrorMessage()
{

	switch (errorCode)
	{
		case BadFileFormat:
		{
			return QString("File format wasn't recognised");
		}

		case BadTagFormat:
		{
			return QString("Tag format -%1- not recognized.").arg(lastTag);
		}

		case UnknownTag:
		{
			return QString("An unknown Tag (%1 in type %2) was read from collection file").arg(lastTag).arg(lastTagType);
		}

		case UnknownTagType:
		{
			return QString("An unknown type of tag (%1) was read from file").arg(lastTagType);
		}

		case WrongTagCount:
		{
			return QString("Tag count claims to be %1").arg(lastTag);
		}

		case CorruptFile:
		{
			return QString("Your collection file was corrupted and ends to Early");
		}
	}
}

const QStringList qtEmc::getLinks()
{
	return list;
}

const bool qtEmc::isValid()
{
	return valid;
}

bool qtEmc::readBinary(QFile &collection)
{
	int fileSize;
	QString fileHash, fileName;
	quint8 rating, sFileSize, tag, tagType;
	quint16 length, mFileSize;
	quint32 lFileSize, llength, tagCount;
	quint64 xlFileSize;

	QDataStream in(&collection);
	in.setByteOrder(QDataStream::LittleEndian);

	in >> emcVersion >> headerTagCount;

	if (in.atEnd())
	{
		errorCode = CorruptFile;
		return FALSE;
	} else if (emcVersion > 0x02)
	{
		return readText(collection);
	}

	for (int i=1; i<=headerTagCount; i++)
	{
		in >> tagType >> tagFormat;

		if (in.atEnd())
		{
			errorCode = CorruptFile;
			return FALSE;
		}

		if (tagFormat != 1)
		{
			lastTag = tag;
			errorCode = BadTagFormat;
			return FALSE;
		}

		in >> tag;

		if (in.atEnd())
		{
			errorCode = CorruptFile;
			return FALSE;
		}

		switch (tag)
		{
			case 0x01: //FT_FILENAME
			{
				in >> length;

				if (in.atEnd())
				{
					errorCode = CorruptFile;
					return FALSE;
				}

				char* buffer = new char[length];
				in.readRawData(buffer, length);

				name = QString(buffer);

				delete [] buffer;
				break;
			}

			case 0x31: //FT_COLLECTIONAUTHOR
			{
				in >> length;

				if (in.atEnd())
				{
					errorCode = CorruptFile;
					return FALSE;
				}

				char* buffer = new char[length];
				in.readRawData(buffer, length);

				author = QString(buffer);

				delete [] buffer;
				break;
			}

			case 0x32: //FT_COLLECTIONAUTHORKEY
			{
				in >> llength;

				if (in.atEnd())
				{
					errorCode = CorruptFile;
					return FALSE;
				}

				char* buffer = new char[llength];
				in.readRawData(buffer, llength);
				authorKey = QString(buffer);

				delete [] buffer;
				break;
			}

			default:
			{
				lastTag = tag;
				errorCode = UnknownTag;
				return FALSE;
			}

			break;
		}
	}

	in >> fileCount;

	if (in.atEnd())
	{
		errorCode = CorruptFile;
		return FALSE;
	}

	for (int i=1; i<=fileCount; i++)
	{
		fileHash.clear();
		fileName.clear();

		in >> tagCount;

		if (in.atEnd())
		{
			errorCode = CorruptFile;
			return FALSE;
		}

		if (tagCount > 5)
		{
			lastTag = tagCount;
			errorCode = WrongTagCount;
			return FALSE;
		}

		for (int j=1; j<=tagCount; j++)
		{
			in >> tag;

			if (in.atEnd())
			{
				errorCode = CorruptFile;
				return FALSE;
			}

			in >> tagType;

			if (in.atEnd())
			{
				errorCode = CorruptFile;
				return FALSE;
			}

			switch (tagType)
			{
				case 0x01: // FT_FILENAME
				{
					switch (tag)
					{
						case 0x82:
						{
							in >> length;

							if (in.atEnd())
							{
								errorCode = CorruptFile;
								return FALSE;
							}

							char* buffer = new char[length];
							in.readRawData(buffer, length);
							fileName = QString().fromLocal8Bit(buffer, length).trimmed();

							if (fileName.length() > length)
							{
								fileName.chop(fileName.length() - length);
							} else if (fileName.length() == length)
							{
								int pos=0;

								for (int k=0; k<length; k++)
								{
									if (fileName.at(k) != buffer[k+pos])
									{
										pos++;
									}
								}

								fileName.chop(pos);
							}

							delete [] buffer;

							if (in.atEnd() && (j != tagCount) && (i =! fileCount))
							{
								errorCode = CorruptFile;
								return FALSE;
							}

							break;
						}

						default:
						{
							if (((tag^0x80) >= 0x11) && ((tag^0x80) <= 0x20))
							{

								length = (tag^0x80) - 0x10;

								char* buffer = new char[length];
								in.readRawData(buffer, length);
								fileName = QString().fromLocal8Bit(buffer, length).trimmed();

								if (fileName.length() > length)
								{
									fileName.chop(fileName.length() - length);
								} else if (fileName.length() == length)
								{
									int pos=0;
	
									for (int k=0; k<length; k++)
									{
										if (fileName.at(k) != buffer[k+pos])
										{
											pos++;
										}
									}

									fileName.chop(pos);
								}
	
								delete [] buffer;
	
								if (in.atEnd() && (j != tagCount) && (i =! fileCount))
								{
									errorCode = CorruptFile;
									return FALSE;
								}
							} else {
								lastTag = tag;
								lastTagType = tagType;
								errorCode = UnknownTag;
								return FALSE;
							}
						}
					}

					break;
				}

				case 0x02: //FT_FILESIZE
				{
					switch (tag)
					{
						case 0x83:
						{
							in >> lFileSize;
							fileSize = lFileSize;

							if (in.atEnd() && (j != tagCount) && (i =! fileCount))
							{
								errorCode = CorruptFile;
								return FALSE;
							}

							break;
						}

						case 0x88:
						{
							in >> mFileSize;
							fileSize = mFileSize;

							if (in.atEnd() && (j != tagCount) && (i =! fileCount))
							{
								errorCode = CorruptFile;
								return FALSE;
							}

							break;
						}

						case 0x89:
						{
							in >> sFileSize;
							fileSize = sFileSize;

							if (in.atEnd() && (j != tagCount) && (i =! fileCount))
							{
								errorCode = CorruptFile;
								return FALSE;
							}

							break;
						}

						case 0x8b:
						{
							in >> xlFileSize;
							fileSize = xlFileSize;

							if (in.atEnd() && (j != tagCount) && (i =! fileCount))
							{
								errorCode = CorruptFile;
								return FALSE;
							}

							break;
						}

						default:
						{
							lastTag = tag;
							lastTagType = tagType;
							errorCode = UnknownTag;
							return FALSE;
						}
					}

					break;
				}

				case 0x28: //FT_FILEHASH
				{
					switch (tag)
					{
						case 0x81: //FT_FILEHASH
						{
							char* buffer = new char[16];
							in.readRawData(buffer, 16);

							if (in.atEnd() && (j != tagCount) && (i =! fileCount))
							{
								errorCode = CorruptFile;
								return FALSE;
							}

							for (int pos = 0; pos < 16; pos++)
							{
								fileHash.append(QString().setNum(((buffer[pos] >> 4) & 0xF), 16));
								fileHash.append(QString().setNum((buffer[pos] & 0x0F), 16));
							}

							delete [] buffer;
							break;
						}

						default:
						{
							lastTag = tag;
							lastTagType = tagType;
							errorCode = UnknownTag;
							return FALSE;
						}
					}

					break;
				}
// The comment is read correctly. It will be stored when I know what to do with it.
				case 0xf6: //FT_FILECOMMENT
				{
					if ((tag^0x80) == 0x02)
					{
						in >> length;

						if (in.atEnd())
						{
							errorCode = CorruptFile;
							return FALSE;
						}
					} else {
						length = ((tag^0x80)-0x10);
					}

					char* buffer = new char[length];
					in.readRawData(buffer, length);

					delete [] buffer;
					break;
				}
// The file-rating is read correctly. It will be stored when I know what to do with it.
				case 0xf7: //FT_FILERATING
				{
					in >> rating;

					break;
				}

				default:
				{
					lastTagType = tagType;
					errorCode = UnknownTagType;
					return FALSE;
				}
			}
		}

		list.append(QString("ed2k://|file|%1|%2|%3|/").arg(fileName).arg(fileSize).arg(fileHash));
	}

	return TRUE;
}

bool qtEmc::readText(QFile &collection)
{
	quint8 character;
	QString tmp;

	collection.seek(0);
	QDataStream in(&collection);
	in.setByteOrder(QDataStream::LittleEndian);

	for (int i=0; i<=6; i++)
	{
		in >> character;
		tmp.append(character);
	}

	if (tmp == "ed2k://")
	{
		while (!in.atEnd())
		{
			in >> character;

			if (character == 0x0d)
			{
				list.append(tmp);
				tmp.clear();
			} else if (character != 0x0a)
			{
				tmp.append(character);
			}
		}
	} else {
			errorCode = BadFileFormat;
			return FALSE;
	}

	return TRUE;
}
