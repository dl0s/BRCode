import bb.cascades 1.4

TextField {
    property bool scopeActive: true
    enabled: scopeActive
    textFormat: TextFormat.Plain
    inputMode: TextFieldInputMode.Default
    input.flags: TextInputFlag.AutoCorrectionOff | TextInputFlag.AutoCapitalizationOff | TextInputFlag.AutoPeriodOff | TextInputFlag.WordSubstitutionOff
}
