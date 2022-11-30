function Controller() {

    // skip unused pages in unistaller
    if (installer.isUninstaller()) {
	installer.setDefaultPageVisible(QInstaller.Introduction, false);
	installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
	installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
    }
}

Controller.prototype.IntroductionPageCallback = function()
{
    var widget = gui.currentPageWidget();

    // expand the package description
    if (widget != null) {
        widget.MessageLabel.setText("Welcome to the Guglielmo Setup Wizard.\n\n" +
		"Guglielmo is a FM and DAB radio tuner with a retro interface and support for a variety of devices.");
    }
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    // skip component selection if only one component
    if (installer.components().length == 1)
	gui.clickButton(buttons.NextButton);
}
