#pragma once

#include <Ole2.h>

class FDropTarget : public IDropTarget
{
public:

	FDropTarget();
	~FDropTarget();

	// IUnknown Begin
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

	virtual ULONG STDMETHODCALLTYPE AddRef(void) override;

	virtual ULONG STDMETHODCALLTYPE Release(void) override;
	// IUnknown End

	// IDropTarget Begin
	virtual HRESULT STDMETHODCALLTYPE DragEnter(
		/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ __RPC__inout DWORD *pdwEffect) override;

	virtual HRESULT STDMETHODCALLTYPE DragOver(
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ __RPC__inout DWORD *pdwEffect) override;

	virtual HRESULT STDMETHODCALLTYPE DragLeave(void) override;

	virtual HRESULT STDMETHODCALLTYPE Drop(
		/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ __RPC__inout DWORD *pdwEffect) override;
	// IDropTarget End

private:
	LONG ref_count = 0;
};