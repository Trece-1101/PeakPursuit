#pragma once

//Debug::Print(FString::Printf(TEXT("%f, %f, %f"), StartOffset.X, StartOffset.Y, StartOffset.Z));

namespace Debug
{
    static void Print(const FString& Msg, const float& Duration = 1.0f, const FColor& Color = FColor::Cyan, const bool& PrintToLog = false, int32 InKey = -1)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(InKey, Duration, Color, Msg);
        }
  
        if (PrintToLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
        }
    }
}