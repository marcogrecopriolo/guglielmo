function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();
	  if (systemInfo.productType === "windows") {
		component.addElevatedOperation("Execute", "@TargetDir@/SDRplay_RSP_API-Windows-3.11.exe");
		component.addElevatedOperation("Delete", "@TargetDir@/SDRplay_RSP_API-Windows-3.11.exe");
    }
}
