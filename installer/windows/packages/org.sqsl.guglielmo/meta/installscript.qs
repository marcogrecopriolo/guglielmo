function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();
	  if (systemInfo.productType === "windows") {
		component.addOperation("CreateShortcut", "@TargetDir@/guglielmo.exe", "@StartMenuDir@/guglielmo.lnk",
		    "workingDirectory=@TargetDir@", "iconPath=@TargetDir@//guglielmo.exe");
    }
}
