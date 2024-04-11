"use client";

import { useState } from "react";
import { Slider } from "@/components/ui/slider";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Lightbulb, LightbulbOff } from "lucide-react";
import { handleCommandSocket } from "@/actions/commandHandler";

interface IPlaygroundCardProps {
  title: string;
  description: string;
  children: React.ReactNode;
}

function PlaygroundCard(props: IPlaygroundCardProps) {
  return (
    <div className="rounded-lg border bg-card text-card-foreground shadow-sm sm:col-span-2">
      <div className="flex flex-col space-y-1.5 p-6">
        <h3 className="text-2xl font-semibold leading-none tracking-tight">
          {props.title}
        </h3>
        <p className="text-sm text-muted-foreground">{props.description}</p>
        {props.children}
      </div>
    </div>
  );
}

type ILightModes = "off" | "on" | "flashing";
type IGyroModes = "x" | "y" | "z";

export default function Dashboard() {
  const [fanSpeed, setFanSpeed] = useState(100);
  const [ledMode, setLedMode] = useState<ILightModes>("off");
  const [gyroReading, setGyroReading] = useState<number | undefined>(undefined);
  const [updateInProgress, setUpdateInProgress] = useState(false);

  async function updateGyroReading(mode: IGyroModes) {
    if (updateInProgress) return;
    setUpdateInProgress(true);
    setGyroReading(undefined);

    try {
      let res;
      switch (mode) {
        case "x":
          res = await handleCommandSocket("get_gyro x");
          break;
        case "y":
          res = await handleCommandSocket("get_gyro y");
          break;
        case "z":
          res = await handleCommandSocket("get_gyro z");
          break;
      }
      console.log(res);
    } catch (error) {
      console.error(error);
    }

    setUpdateInProgress(false);
  }

  async function updateLEDMode(mode: ILightModes) {
    if (updateInProgress) return;
    setUpdateInProgress(true);
    setLedMode(mode);

    try {
      let res;
      switch (mode) {
        case "off":
          res = await handleCommandSocket("set_led off");
          break;
        case "on":
          res = await handleCommandSocket("set_led on");
          break;
        case "flashing":
          res = await handleCommandSocket("set_led flashing");
          break;
      }
      console.log(res);
    } catch (error) {
      console.error(error);
    }

    setUpdateInProgress(false);
  }

  async function updateFanSpeed(value: number) {
    if (updateInProgress) return;
    setUpdateInProgress(true);

    try {
      const res = await handleCommandSocket(`set_fan ${value}`);
      console.log(res);
    } catch (error) {
      console.error(error);
    }

    setUpdateInProgress(false);
  }

  return (
    <div className="flex w-full flex-col">
      <div className="grid gap-4 sm:grid-cols-2 md:grid-cols-4 lg:grid-cols-6 xl:grid-cols-8">
        <PlaygroundCard
          title="Fan Control"
          description="Move the slider to control the fan speed"
        >
          <span
            className={`text-lg font-semibold ${
              updateInProgress && "text-muted-foreground"
            }`}
          >
            {fanSpeed}%
          </span>
          <Slider
            defaultValue={[fanSpeed]}
            onValueChange={(values) => setFanSpeed(values[0])}
            onValueCommit={() => updateFanSpeed(fanSpeed)}
            disabled={updateInProgress}
            step={1}
            min={0}
            max={100}
          />
        </PlaygroundCard>
        <PlaygroundCard
          title="LED Mode Control"
          description="Select which mode the LED should be in"
        >
          <div className="pt-4 flex justify-between items-center">
            <Tabs
              defaultValue="off"
              onValueChange={(value) => updateLEDMode(value as ILightModes)}
            >
              <TabsList>
                <TabsTrigger value="off" disabled={updateInProgress}>
                  Off
                </TabsTrigger>
                <TabsTrigger value="on" disabled={updateInProgress}>
                  On
                </TabsTrigger>
                <TabsTrigger value="flashing" disabled={updateInProgress}>
                  Flashing
                </TabsTrigger>
              </TabsList>
            </Tabs>
            {ledMode === "flashing" && (
              <Lightbulb className="w-8 h-8 text-yellow-500 animate-flash" />
            )}
            {ledMode === "off" && <LightbulbOff className="w-8 h-8" />}
            {ledMode === "on" && (
              <Lightbulb className="w-8 h-8 text-yellow-500" />
            )}
          </div>
        </PlaygroundCard>
        <PlaygroundCard
          title="Gyro Position"
          description="Select an axis and get the current position"
        >
          <div className="pt-4 flex justify-between items-center">
            <Tabs
              defaultValue="x"
              onValueChange={(value) => updateGyroReading(value as IGyroModes)}
            >
              <TabsList>
                <TabsTrigger value="x" disabled={updateInProgress}>
                  X Axis
                </TabsTrigger>
                <TabsTrigger value="y" disabled={updateInProgress}>
                  Y Axis
                </TabsTrigger>
                <TabsTrigger value="z" disabled={updateInProgress}>
                  Z Axis
                </TabsTrigger>
              </TabsList>
            </Tabs>
            {gyroReading ? (
              <span className="text-lg font-semibold">{gyroReading}</span>
            ) : (
              <span className="text-lg font-semibold">&mdash;</span>
            )}
          </div>
        </PlaygroundCard>
      </div>
    </div>
  );
}
