<template>
    <p>Remote is <code>{{ remote }}</code></p>

    <div id="terminal"></div>
</template>

<script>
import { Terminal } from 'xterm';
import { WebglAddon } from '@xterm/addon-webgl';
import { FitAddon } from '@xterm/addon-fit';
import { AttachAddon } from '@xterm/addon-attach';
import 'xterm/css/xterm.css';

export default {
  name: 'RemoteTerm',
  props: {
    remote: String
  },
  data() {
    return {
      errored: false
    }
  },
  mounted() {
    const term = new Terminal({
      'theme': { background: '#000000' },
      'convertEol': true,
    });

    const socket = new WebSocket(this.remote);

    const fitAddon = new FitAddon();

    term.loadAddon(new WebglAddon());
    term.loadAddon(new AttachAddon(socket));
    term.loadAddon(fitAddon);

    fitAddon.fit();

    term.open(document.getElementById('terminal'));
    socket.addEventListener('close', function () {
      if (!this.errored) {
        term.write("\r\n\r\n\x1B[0mNO CARRIER\r\n\r\n");
      }
      this.errored = true;
    });
    socket.addEventListener('error', function () {
      if (!this.errored) {
        term.write("\r\n\r\n\x1B[0mNO CARRIER\r\n\r\n");
      }
      this.errored = true;
    });

    // socket.onmessage = function (event) {
    //   console.log("WebSocket message received:", event.value);
    // };

  }
}
</script>

<script setup>
</script>

<style scoped>
h3 {
  margin: 40px 0 0;
}
ul {
  list-style-type: none;
  padding: 0;
}
li {
  display: inline-block;
  margin: 0 10px;
}
a {
  color: #42b983;
}

div#terminal {
  min-height: 100%;
  height: 100%;
  margin: auto;
  text-align: left;
}

</style>
