#pragma once
#include "object.h"
#include <functional>

/*
	Shawn Xiong @2/15/2023
	Tensor is a multiple dimensional array whith same data type
	compare with list which hold each item at least take 16-bytes
	(X::Value), but Tensor will reduce memory usage
	for example Tensor of float, each item just take 4-bytes
	and Tensor also can hold X::Value as its item, then item can be
	vairant data type
	in this Implementation, use templeate to define this Tensor
	class, and will register for diffrent type of Tensor
	include: 
		int, float, double,complex and X::Value
	maybe also include char as item to reduce memeory 
	in some case use as flags
*/
namespace X
{
	namespace Data
	{
		struct TensorDim
		{
			long long size;
			long long stride;
			long long dimProd;
		};
		enum class Tensor_Operator
		{
			None,Add,Minus,Mul,Div
		};
		using TensorIterateProc = std::function<void(std::vector<long long>& indices)>;
		class Tensor:
			virtual public XTensor,
			virtual public Object
		{
			//for tensor operator
			X::Value m_rightVal; //maybe a Tensor or TensorOperator
			Tensor_Operator m_op = Tensor_Operator::None;
			void SetRightVal(X::Value& val, Tensor_Operator op)
			{
				m_rightVal = val;
				m_op = op;
			}
			//for data tensor
			char* m_data=nullptr;
			std::vector<TensorDim> m_dims;
			TensorDataType m_dataType;


			long long GetItemSize()
			{
				long long size = 1;
				switch (m_dataType)
				{
				case X::TensorDataType::BOOL:
				case X::TensorDataType::BYTE:
				case X::TensorDataType::UBYTE:
					size = 1;
					break;
				case X::TensorDataType::SHORT:
				case X::TensorDataType::USHORT:
				case X::TensorDataType::HALFFLOAT:
					size = 2;
					break;
				case X::TensorDataType::INT:
				case X::TensorDataType::UINT:
					size = 4;
					break;
				case X::TensorDataType::LONGLONG:
				case X::TensorDataType::ULONGLONG:
					size = 8;
					break;
				case X::TensorDataType::FLOAT:
					size = 4;
					break;
				case X::TensorDataType::DOUBLE:
					size = 8;
					break;
				case X::TensorDataType::CFLOAT:
					size = 8;
					break;
				case X::TensorDataType::CDOUBLE:
					size = 16;
					break;
				default:
					break;
				}
				return size;
			}
			long long GetCount()
			{
				long long itemCnt = 1;
				for (auto& d : m_dims)
				{
					itemCnt *= d.size;
				}
				return itemCnt;
			}
			void DeepCopyDataFromList(List* pList,std::vector<long long>& indices,int level);
			X::Value GetDataWithIndices(std::vector<long long>& indices);
			void CalcDimProd()
			{
				int nd = (int)m_dims.size();
				long long a = 1;
				m_dims[nd - 1].dimProd = a;
				for (int i = nd - 1; i >= 1; i--)
				{
					a *= m_dims[i].size;
					m_dims[i - 1].dimProd = a;
				}
			}
		public:
			static void cleanup();
			Tensor();
			~Tensor();

			//this function only return first dim's size
			//because debug will use it as first level
			virtual long long Size() override
			{
				if (m_dims.size() > 0)
				{
					return m_dims[0].size;
				}
				else
				{
					return 0;
				}
			}
			virtual long long GetDataSize()
			{
				return GetCount() * GetItemSize();
			}
			virtual char* GetData() override
			{
				return m_data;
			}
			virtual void SetShape(std::vector<int> shapes) override
			{
				for (auto i : shapes)
				{
					m_dims.push_back(TensorDim{ i,i });
				}
			}
			void SetDataWithIndices(std::vector<long long>& indices, X::Value& val);
			inline X::Value asType(int type)
			{
				TensorDataType dt = (TensorDataType)type;
				Tensor* pNewTensor = new Tensor();
				pNewTensor->SetDataType(dt);
				int dimCnt = (int)m_dims.size();
				std::vector<int> dims;
				for (int i = 0; i < dimCnt; i++)
				{
					dims.push_back(m_dims[i].size);
				}
				pNewTensor->SetShape(dims);
				X::Value initData;
				pNewTensor->Create(initData);
				auto it_proc = [pNewTensor, this, dimCnt](std::vector<long long>& indices)
				{
					X::Value val = GetDataWithIndices(indices);
					pNewTensor->SetDataWithIndices(indices, val);
				};
				IterateAll(it_proc);
				return X::Value(pNewTensor);

			}
			inline X::Value permute(std::vector<int> axes)
			{
				Tensor* pNewTensor = new Tensor();
				pNewTensor->SetDataType(m_dataType);
				int dimCnt = (int)m_dims.size();
				std::vector<int> dims;
				for (int i=0;i< dimCnt;i++)
				{
					dims.push_back(m_dims[axes[i]].size);
				}
				pNewTensor->SetShape(dims);
				X::Value initData;
				pNewTensor->Create(initData);
				auto it_proc = [pNewTensor,this, dimCnt, axes](std::vector<long long>& indices)
				{
					std::vector<long long> target_indices;
					for (int i = 0; i < dimCnt; i++)
					{
						target_indices.push_back(indices[axes[i]]);
					}
					X::Value val = GetDataWithIndices(indices);
					pNewTensor->SetDataWithIndices(target_indices, val);
				};
				IterateAll(it_proc);
				return X::Value(pNewTensor);
			}
			inline void IterateAll(TensorIterateProc proc)
			{
				std::vector<long long> indices;
				indices.resize(m_dims.size());
				IterateLoop(indices, proc, 0);
			}
			//before call,indices need has same size of m_dims
			inline void IterateLoop(std::vector<long long>&indices, TensorIterateProc proc,int level=0)
			{
				if (m_dims.size() == 0)
				{
					return;
				}
				int lastDim = (int)m_dims.size()-1;
				auto& dim = m_dims[level];
				for (long long i = 0; i < dim.size; i++)
				{
					indices[level] = i;
					if (lastDim == level)
					{
						proc(indices);
					}
					else
					{
						IterateLoop(indices, proc, level + 1);
					}
				}
			}
			virtual void SetDataType(TensorDataType t) override
			{
				m_dataType = t;
			}
			virtual Tensor& operator *=(X::Value& r) override;
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
			virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				std::string itemName, X::Value& val) override;
			virtual bool Create(X::Value& initData) override;
			static AST::Scope* GetBaseScope();
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			virtual std::string ToString(bool WithFormat = false) override
			{
				AutoLock(m_lock);
				std::string strShapes;
				char v[1000];
				int dcnt = (int)m_dims.size();
				for (int i=0;i<dcnt;i++)
				{
					auto& d = m_dims[i];
					if (i > 0)
					{
						strShapes += ",";
					}
					snprintf(v, sizeof(v), "%ld",d.size);
					strShapes += v;
				}
				std::string strOut = "Tensor(size=(" + strShapes + "),";
				strOut +="[todo:output data";
				strOut += "]";
				strOut += ")";
				return strOut;
			}
		};
	}
}