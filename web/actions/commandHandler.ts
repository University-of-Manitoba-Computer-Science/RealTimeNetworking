"use server";

import stateStore from "@/lib/globalState";
import net from "node:net";

export interface ICommandResponse {
  time: number;
  message: string;
  success: boolean;
}

async function handleCommand(input: string): Promise<ICommandResponse> {
  const store = stateStore;
  const parts = input.split(" ");
  const command = parts[0];
  const args = parts.slice(1);

  switch (command) {
    case "help":
      return {
        time: Date.now(),
        message: "Available commands: help, set_light, get_light",
        success: true,
      };
    case "set_light":
      if (args[0] === "on") {
        store.setLight(true);
        return {
          time: Date.now(),
          message: "Light has been turned on",
          success: true,
        };
      } else if (args[0] === "off") {
        store.setLight(false);
        return {
          time: Date.now(),
          message: "Light has been turned off",
          success: true,
        };
      } else {
        return {
          time: Date.now(),
          message: "Invalid argument. Please use 'on' or 'off'",
          success: false,
        };
      }
    case "get_light":
      return {
        time: Date.now(),
        message: store.getLight() ? "Light is on" : "Light is off",
        success: true,
      };
    default:
      return {
        time: Date.now(),
        message: `Unknown command: ${command}`,
        success: false,
      };
  }
}

async function handleCommandSocket(command: string) {
  return new Promise<ICommandResponse>((resolve) => {
    const client = net.connect(
      {
        port: 8080,
        host: "192.168.1.1",
      },
      async () => {
        await new Promise((resolve) => {
          setTimeout(resolve, 1000);
        });
        const payload = Buffer.from(command + "\n");
        client.write(payload);
      }
    );

    client.on("data", (data) => {
      resolve({
        time: Date.now(),
        message: data.toString(),
        success: true,
      });
      client.end();
    });

    client.on("error", (error) => {
      resolve({
        time: Date.now(),
        message: `Error: ${error.message}`,
        success: false,
      });
    });

    setTimeout(() => {
      resolve({
        time: Date.now(),
        message: "Timeout: Command execution took too long",
        success: false,
      });
      client.end();
    }, 10000);
  });
}

export { handleCommand, handleCommandSocket };
