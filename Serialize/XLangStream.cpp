#include "XLangStream.h"
#include <stdexcept>
#include "object.h"

namespace X 
{
	XLangStream::XLangStream()
	{
		m_streamKey = 0;// GrusJitHost::I().RegisterStream(this);
	}

	XLangStream::~XLangStream()
	{
		//GrusJitHost::I().UnregisterStream(m_streamKey);
		m_size = 0;
		curPos = { 0,0 };
	}
	bool XLangStream::FullCopyTo(char* buf, STREAM_SIZE bufSize)
	{
		if (bufSize < Size())
		{
			return false;
		}
		int blkNum = BlockNum();
		if (blkNum == 0)
		{//empty
			return true;
		}
		for (int i = 0; i < blkNum; i++)
		{
			blockInfo& blk = GetBlockInfo(i);
			memcpy(buf, blk.buf, blk.data_size);
			buf += blk.data_size;
		}
		return true;
	}
	bool XLangStream::CopyTo(char* buf, STREAM_SIZE size)
	{
		blockIndex bi = curPos;
		STREAM_SIZE leftSize = size;
		char* pOutputData = buf;
		int curBlockIndex = bi.blockIndex;
		STREAM_SIZE blockOffset = bi.offset;
		while (leftSize > 0)
		{
			if (curBlockIndex >= BlockNum())
			{
				return false;
			}
			blockInfo& curBlock = GetBlockInfo(curBlockIndex);

			STREAM_SIZE restSizeInBlock = curBlock.data_size - blockOffset;
			STREAM_SIZE copySize = leftSize < restSizeInBlock ? leftSize : restSizeInBlock;
			if (buf != nullptr)
			{
				memcpy(pOutputData, curBlock.buf + blockOffset, copySize);
			}
			pOutputData += copySize;
			blockOffset += copySize;
			leftSize -= copySize;
			if (leftSize > 0)//need next block
			{
				if (!MoveToNextBlock())
				{
					return false;
				}
				blockOffset = 0;
				curBlockIndex++;
			}
		}
		curPos.blockIndex = curBlockIndex;
		curPos.offset = blockOffset;

		return true;
	}

	bool XLangStream::appendchar(char c)
	{
		int blkNum = BlockNum();
		if (curPos.blockIndex >= blkNum)
		{
			if (!NewBlock())
			{
				return false;
			}
		}
		blockInfo& curBlock = GetBlockInfo(curPos.blockIndex);
		if (curPos.offset == curBlock.block_size)
		{
			curPos.blockIndex++;
			if (curPos.blockIndex >= blkNum)
			{
				if (!NewBlock())
				{
					return false;
				}
			}
			curPos.offset = 0;
		}
		*(curBlock.buf + curPos.offset) = c;
		curPos.offset++;
		if (!m_InOverrideMode)
		{
			curBlock.data_size++;
			m_size++;
		}
		return true;
	}
	bool XLangStream::fetchchar(char& c)
	{
		int blkNum = BlockNum();
		if (curPos.blockIndex >= blkNum)
		{
			return false;
		}
		blockInfo& curBlock = GetBlockInfo(curPos.blockIndex);
		if (curPos.offset == curBlock.block_size)
		{
			MoveToNextBlock();
			curPos.blockIndex++;
			if (curPos.blockIndex >= blkNum)
			{
				return false;
			}
			curPos.offset = 0;
		}
		c = *(curBlock.buf + curPos.offset);
		curPos.offset++;
		return true;
	}
	bool XLangStream::fetchstring(std::string& str)
	{
		char ch = 0;
		bool bRet = true;
		while (bRet)
		{
			bRet = fetchchar(ch);
			if (ch == 0)
			{
				break;
			}
			if (bRet)
			{
				str += ch;
			}
		}
		return bRet;
	}
	bool XLangStream::append(char* data, STREAM_SIZE size)
	{
		blockIndex bi = curPos;
		STREAM_SIZE leftSize = size;
		char* pInputData = data;
		int curBlockIndex = bi.blockIndex;
		STREAM_SIZE blockOffset = bi.offset;

		while (leftSize > 0)
		{
			if (curBlockIndex >= BlockNum())
			{
				if (!NewBlock())
				{
					return false;
				}
			}
			blockInfo& curBlock = GetBlockInfo(curBlockIndex);
			STREAM_SIZE restSizeInBlock = curBlock.block_size - blockOffset;
			STREAM_SIZE copySize = leftSize < restSizeInBlock ? leftSize : restSizeInBlock;
			memcpy(curBlock.buf + blockOffset, pInputData, copySize);
			if (!m_InOverrideMode)
			{
				curBlock.data_size += copySize;
			}
			pInputData += copySize;
			blockOffset += copySize;
			leftSize -= copySize;
			if (leftSize > 0)//need next block
			{
				blockOffset = 0;
				curBlockIndex++;
			}
		}
		curPos.blockIndex = curBlockIndex;
		curPos.offset = blockOffset;
		if (!m_InOverrideMode)
		{
			m_size += size;
		}
		return true;
	}

	STREAM_SIZE XLangStream::CalcSize(blockIndex pos)
	{
		if (BlockNum() <= pos.blockIndex)
		{
			return -1;
		}
		STREAM_SIZE size = 0;
		for (int i = 0; i < pos.blockIndex - 1; i++)
		{
			blockInfo& curBlock = GetBlockInfo(i);
			size += curBlock.data_size;
		}
		//last block use offset,not datasize
		size += pos.offset;
		return size;
	}

	void XLangStream::Refresh()
	{
		if (m_pProvider)
		{
			m_pProvider->Refresh();
		}
	}

	int XLangStream::BlockNum()
	{
		return m_pProvider ? m_pProvider->BlockNum() : 0;
	}

	blockInfo& XLangStream::GetBlockInfo(int index)
	{
		if (m_pProvider)
		{
			return m_pProvider->GetBlockInfo(index);
		}
		else
		{
			static blockInfo blk = { 0 };
			return blk;
		}
	}

	bool XLangStream::NewBlock()
	{
		return m_pProvider ? m_pProvider->NewBlock() : false;
	}

	bool XLangStream::MoveToNextBlock()
	{
		return m_pProvider ? m_pProvider->MoveToNextBlock() : false;
	}
	XLangStream& XLangStream::operator<<(X::Value& v)
	{
		auto t = v.GetType();
		(*this) << (char)t;
		switch (t)
		{
		case X::ValueType::Invalid:
			break;
		case X::ValueType::None:
			break;
		case X::ValueType::Int64:
			(*this) << v.GetLongLong();
			break;
		case X::ValueType::Double:
			(*this) << v.GetDouble();
			break;
		case X::ValueType::Object:
		{
			X::Data::Object* pObj = dynamic_cast<X::Data::Object*>(v.GetObj());
			pObj->ToBytes(*this);
		}
		break;
		case X::ValueType::Str:
			(*this) << v.ToString();
			break;
		default:
			break;
		}
		return *this;

	}
	XLangStream& XLangStream::operator>>(X::Value& v)
	{
		char ch;
		(*this) >> ch;
		X::ValueType t = (X::ValueType)ch;
		v.SetType(t);
		switch (t)
		{
		case X::ValueType::Invalid:
			break;
		case X::ValueType::None:
			break;
		case X::ValueType::Int64:
		{
			long long l;
			(*this) >> l;
			v.SetLongLong(l);
		}
			break;
		case X::ValueType::Double:
		{
			double d;
			(*this) >> d;
			v.SetDouble(d);
		}
			break;
		case X::ValueType::Object:
		{
			X::Data::Object* pObj = dynamic_cast<X::Data::Object*>(v.GetObj());
			pObj->FromBytes(*this);
		}
		break;
		case X::ValueType::Str:
		{
			std::string s;
			(*this) >> s;
			v.SetString(s);
		}
			break;
		default:
			break;
		}
		return *this;

	}
}