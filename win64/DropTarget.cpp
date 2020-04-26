#include "DropTarget.h"

#include "app.h"

FDropTarget::FDropTarget()
{
	AddRef();
}

FDropTarget::~FDropTarget()
{
	Release();
}

HRESULT FDropTarget::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (riid == IID_IUnknown || riid == IID_IDropTarget)
	{
		*ppvObject = static_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

ULONG FDropTarget::AddRef()
{
	return InterlockedIncrement(&RefCount);
}

ULONG FDropTarget::Release()
{
	LONG new_ref_count = InterlockedDecrement(&RefCount);
	if (new_ref_count == 0) delete this;
	return new_ref_count;
}

HRESULT FDropTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	*pdwEffect &= DROPEFFECT_COPY;
	return S_OK;
}

HRESULT FDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	*pdwEffect &= DROPEFFECT_COPY;
	return S_OK;
}

HRESULT FDropTarget::DragLeave()
{
	return S_OK;
}

HRESULT FDropTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT,
					-1, TYMED_HGLOBAL };
	STGMEDIUM stgm;
	if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm)))
	{
		HDROP hdrop = reinterpret_cast<HDROP>(stgm.hGlobal);
		UINT cFiles = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
		for (UINT i = 0; i < cFiles; i++)
		{
			TCHAR szFile[MAX_PATH];
			UINT cch = DragQueryFile(hdrop, i, szFile, MAX_PATH);
			if (cch > 0 && cch < MAX_PATH)
			{
				App::OpenAdditionalFile(szFile);
			}
		}
		ReleaseStgMedium(&stgm);
	}


	*pdwEffect &= DROPEFFECT_COPY;
	return S_OK;
}