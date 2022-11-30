function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();
	  if (systemInfo.productType === "windows") {
		component.addElevatedOperation("Execute", "@TargetDir@/zadig.exe");
		component.addElevatedOperation("Delete", "@TargetDir@/zadig.exe");
    }
}
