using UnrealBuildTool;
using System.Collections.Generic;
using System;

public class ClubXServerTarget : TargetRules
{
    public ClubXServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V2;

        ExtraModuleNames.AddRange(new string[] { "ClubX" });
    }
}