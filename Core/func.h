#pragma once
#include "exp.h"
#include "scope.h"
#include "block.h"
#include "var.h"

namespace X
{
namespace AST
{
typedef bool (*U_FUNC) (Runtime* rt,void* pContext,
	std::vector<Value>& params,
	std::unordered_map<std::string, AST::Value>& kwParams,
	Value& retValue);
class Func :
	public Block,
	public Scope
{
protected:
	String m_Name = { nil,0 };
	int m_Index = -1;//index for this Var,set by compiling
	int m_positionParamCnt = 0;
	int m_paramStartIndex = 0;
	int m_IndexOfThis = -1;//for THIS pointer
	bool m_needSetHint = false;
	Expression* Name = nil;
	List* Params = nil;
	Expression* RetType = nil;
	void SetName(Expression* n)
	{
		Var* vName = dynamic_cast<Var*>(n);
		if (vName)
		{
			m_Name = vName->GetName();
		}
		Name = n;
		if (Name)
		{
			Name->SetParent(this);
		}
	}

	virtual void ScopeLayout() override;
	virtual void SetParams(List* p)
	{
		if (m_needSetHint)
		{
			if (p)
			{
				auto& list = p->GetList();
				if (list.size() > 0)
				{
					auto exp = list[0];
					SetHint(exp->GetStartLine(), exp->GetEndLine(),
						exp->GetCharPos());
				}
			}
		}
		Params = p;
		if (Params)
		{
			Params->SetParent(this);
		}
	}
public:
	Func() :
		Scope()
	{
		m_type = ObType::Func;
	}
	~Func()
	{
		if (Params) delete Params;
		if (RetType) delete RetType;
	}
	void NeedSetHint(bool b) { m_needSetHint = b; }
	Expression* GetName() { return Name; }
	String& GetNameStr() { return m_Name; }
	virtual Scope* GetParentScope() override
	{
		return GetScope();
	}
	virtual void SetR(Expression* r) override
	{
		if (r->m_type == AST::ObType::Pair)
		{//have to be a pair
			AST::PairOp* pair = (AST::PairOp*)r;
			AST::Expression* paramList = pair->GetR();
			if (paramList)
			{
				if (paramList->m_type != AST::ObType::List)
				{
					AST::List* list = new AST::List(paramList);
					SetParams(list);
				}
				else
				{
					SetParams((AST::List*)paramList);
				}
			}
			pair->SetR(nil);//clear R, because it used by SetParams
			AST::Expression* l = pair->GetL();
			if (l)
			{
				SetName(l);
				pair->SetL(nil);
			}
			//content used by func,and clear to nil,
			//not be used anymore, so delete it
			delete pair;
		}
	}

	void SetRetType(Expression* p)
	{
		RetType = p;
		if (RetType)
		{
			RetType->SetParent(this);
		}
	}
	virtual int AddOrGet(std::string& name, bool bGetOnly)
	{
		int retIdx = Scope::AddOrGet(name, bGetOnly);
		return retIdx;
	}
	virtual bool Call(Runtime* rt, void* pContext,
		std::vector<Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		Value& retValue);
	virtual bool Run(Runtime* rt, void* pContext,
		Value& v, LValue* lValue = nullptr) override;
};
class ExternFunc
	:public Func
{
	std::string m_funcName;
	U_FUNC m_func;
public:
	ExternFunc(std::string& funcName, U_FUNC func)
	{
		m_funcName = funcName;
		m_func = func;
	}
	inline virtual bool Call(Runtime* rt, void* pContext,
		std::vector<Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		Value& retValue) override
	{
		return m_func ? m_func(rt,
			pContext, params, kwParams, retValue) : false;
	}
};

}
}