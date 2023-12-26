<template>
  <div class="box">
    <div class="row header">
      <img :src="logo" alt="rosco_m68k logo">
    </div>
    <div class="row content">
      <RemoteTerm remote="ws://pi.hole:8000"/>
    </div>
    <div class="row footer">
      Copyright &copy; and whatnot...
    </div>
  </div>
</template>

<script>
import roscologo from './assets/logo-inverted.png'
import RemoteTerm from './components/RemoteTerm.vue'

export default {
  name: 'App',
  components: {
    RemoteTerm
  },
  data() {
    return {
      logo: roscologo
    }
  }
}
</script>

<style>
html,
body {
  height: 100%;
  margin: 0;
}

.box {
  display: flex;
  flex-flow: column;
  height: 100%;
}

.box .row {
  border: 1px dotted grey;
}

.box .row.header {
  flex: 0 1 auto;
  /* The above is shorthand for:
  flex-grow: 0,
  flex-shrink: 1,
  flex-basis: auto
  */
}

.box .row.content {
  flex: 1 1 auto;
}

.box .row.footer {
  flex: 0 1 40px;
}

#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}
</style>
