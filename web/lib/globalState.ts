class StateStore {
  light: boolean;

  constructor() {
    this.light = false;
  }
  setLight(state: boolean) {
    this.light = state;
  }
  getLight() {
    return this.light;
  }
}

let stateStore: StateStore;

if (process.env.NODE_ENV === "production") {
  stateStore = new StateStore();
} else {
  // @ts-ignore
  if (!global.stateStore) {
    // @ts-ignore
    global.stateStore = new StateStore();
  }
  // @ts-ignore
  stateStore = global.stateStore;
}

export default stateStore;
