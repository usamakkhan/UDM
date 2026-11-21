// Included inside udm after Form and CaptureDialogOwner.
inline bool presentCancellationFailure(CWnd* main,Manager& manager,PendingFileInfoInbox& inbox,PendingFileInfoInbox::CancellationFailure failure){
 if(!failure)return false;
 CaptureDialogOwner owner(main,true);Form dialog("Cancellation failed",340,170,owner.get(main));dialog.dialogUnits=true;
 bool saving=false,decisionGuard=false,resolved=false;CWnd* retry=nullptr;CWnd* close=nullptr;CWnd* detail=nullptr;CWnd* status=nullptr;
 struct ReleaseGuard {Manager& manager;bool& owned;~ReleaseGuard(){if(owned)manager.fileInfoDecisionPending=false;}} guard{manager,decisionGuard};
 const auto closeUnresolved=[&]{if(saving)return;inbox.dismissCancellationFailure(failure);dialog.close(IDCANCEL);};
 const auto retryCancellation=[&]{
  if(saving)return;bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another download is being saved. Try again shortly.");decisionGuard=true;
  if(!inbox.retryCancellation(failure)){manager.fileInfoDecisionPending=false;decisionGuard=false;auto current=inbox.cancellationFailure(failure.operation);if(!current){dialog.close(IDCANCEL);return;}failure=std::move(current);return;}
  saving=true;retry->EnableWindow(FALSE);close->EnableWindow(FALSE);status->SetWindowText(L"Cancelling...");
 };
 dialog.init=[&]{
  dialog.label("File: "+str(failure.operation->view->data,"FileName","Browser download"),12,10,316,24);
  dialog.label("UDM could not finish cancelling this download. It may still be in your download list.",12,37,316,28);
  detail=dialog.edit(failure.message,12,68,316,48,true,true);status=dialog.label("Cancellation could not be confirmed.",12,122,316,14);
  retry=dialog.button("&Retry cancellation",166,146,106,retryCancellation,16);close=dialog.button("&Close",278,146,50,closeUnresolved,16);dialog.defaultButton(retry);showBrowserOwnedDialog(dialog,owner.get(main));
 };
 dialog.accept=retryCancellation;dialog.cancel=closeUnresolved;
 dialog.pulse=[&]{
  if(!saving||!failure.operation->cancelFinished.load(std::memory_order_acquire))return;
  saving=false;manager.fileInfoDecisionPending=false;decisionGuard=false;
  if(!failure.operation->cancelFailed){resolved=true;dialog.close(IDOK);return;}
  auto current=inbox.cancellationFailure(failure.operation);if(!current){dialog.close(IDCANCEL);return;}failure=std::move(current);detail->SetWindowText(cs(failure.message));status->SetWindowText(L"Cancellation could not be confirmed.");retry->EnableWindow(TRUE);close->EnableWindow(TRUE);
 };
 if(dialog.DoModal()==-1)throw std::runtime_error("Cannot open cancellation recovery.");return resolved;
}
