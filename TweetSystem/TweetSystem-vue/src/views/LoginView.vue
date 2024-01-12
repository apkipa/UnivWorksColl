<script setup>
import { ref, reactive, VueElement } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()

const loginForm = reactive({
  username: '',
  password: '',
})

const isLogining = ref(false)
const isRegistering = ref(false)

const doBoolSwitchAsync = (b, f) => async () => {
  b.value = true
  try {
    return await f()
  } catch (e) {
    throw e
  } finally {
    b.value = false 
  }
};

const onSubmitRegister = doBoolSwitchAsync(isRegistering, async () => {
  const resp = await client.authRegister(loginForm.username, loginForm.password);
  ElMessage.success('注册成功，请再次点击登录')
})
const onSubmitLogin = doBoolSwitchAsync(isLogining, async () => {
  const resp = await client.authLogin(loginForm.username, loginForm.password);
  router.push('/home')
  await client.updateStore()
})

</script>

<template>
  <Teleport to="#modal-container">
    <div class="background-overlay">
      <div class="modal-container">
        <div id="login-box">
          <div id="login-box-header">登录到推文系统</div>
          <el-divider />
          <el-form id="login-box-form" :model="loginForm" label-width="auto" label-position="left" @keyup.enter.native="onSubmitLogin">
            <el-form-item label="用户名">
              <el-input v-model="loginForm.username"/>
            </el-form-item>
            <el-form-item label="密码">
              <el-input v-model="loginForm.password" type="password"/>
            </el-form-item>
            <el-form-item>
              <el-button style="width: 48%;" type="primary" @click="onSubmitRegister" :loading="isRegistering">注册</el-button>
              <el-button style="width: 48%; margin-left: 4%;" @click="onSubmitLogin" :loading="isLogining">登录</el-button>
            </el-form-item>
          </el-form>
        </div>
      </div>
    </div>
  </Teleport>
</template>

<style scoped>
.background-overlay {
  background: #00000080;
  width: 100%;
  height: 100%;
  pointer-events: auto;
}
.modal-container {
  position: absolute;
  margin-left: 32vw;
  margin-top: 10vh;
  /* padding-top: 20vh; */
  width: 36vw;
  height: auto;
  background: white;
  border-radius: 15px;
}
#login-box {
  padding-left: 2em;
  padding-right: 2em;
  padding-top: 1.2em;
  padding-bottom: 0.4em;
  background-color: white;
  border-radius: 1em;
  /* box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19); */
}
#login-box-header {
  text-align: center;
  font-size: x-large;
  margin-bottom: -4px;
}
</style>
