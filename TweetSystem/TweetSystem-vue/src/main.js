import './assets/main.css'
import './assets/styles.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import { ElMessage, ElMessageBox } from 'element-plus'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'
import InfiniteScroll from "element-plus";
import VuePageStack from 'vue-page-stack';
import StackKeepAlive from 'stack-keep-alive'

import App from './App.vue'
import router from './router'

const app = createApp(App)

for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component)
}

app.use(ElementPlus)
// app.use(InfiniteScroll)
app.use(createPinia())
app.use(router)
app.use(StackKeepAlive)
// app.use(VuePageStack, { router })

/* import the fontawesome core */
import { library } from '@fortawesome/fontawesome-svg-core'

/* import font awesome icon component */
import { FontAwesomeIcon } from '@fortawesome/vue-fontawesome'

/* import specific icons */
// import { faTwitter } from '@fortawesome/free-brands-svg-icons'
// import { faUserSecret } from '@fortawesome/free-solid-svg-icons'
import { far } from '@fortawesome/free-regular-svg-icons'
import { fas } from '@fortawesome/free-solid-svg-icons'
import { fab } from '@fortawesome/free-brands-svg-icons'
library.add(far, fas, fab)

/* add icons to the library */
// library.add(faTwitter, faUserSecret)
app.component('font-awesome-icon', FontAwesomeIcon)

app.config.errorHandler = (e) => {
  // ElMessage.error(`发生了未处理的错误: ${e}`);
  console.log(e);
  ElMessage.error(`${e}`);
};

app.mount('#app')
