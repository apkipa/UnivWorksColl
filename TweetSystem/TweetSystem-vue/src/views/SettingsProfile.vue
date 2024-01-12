<script setup>
import { ref, reactive, computed, VueElement, nextTick, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'
import PostItem from '../components/PostItem.vue'

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()

const isSubmitting = ref(false)

const doBoolSwitchAsync = (b, f) => async () => {
  b.value = true
  try {
    return await f()
  } catch (e) {
    throw e
  } finally {
    b.value = false 
  }
}

const profileForm = reactive({
  nickName: '',
  introduction: '',
})

const onCloseModal = async () => {
  router.back()
}

const onSubmitProfile = doBoolSwitchAsync(isSubmitting, async () => {
  const result = await client.authUpdateInfo(
    profileForm.nickName,
    profileForm.introduction,
  );

  // router.replace(router.options.history.state.back)
  // router.back()
  // router.go(0)
  router.push(router.options.history.state.back);
});

(async function() {
  const userInfo = await client.userGetInfo()
  profileForm.nickName = userInfo.nickname
  profileForm.introduction = userInfo.introduction
  console.log(userInfo.nickname)
})();

</script>

<template>
  <Teleport to="#modal-container">
    <div class="background-overlay">
      <div class="modal-container">
        <div class="profile-box">
          <!-- <div id="post-box-header">登录到推文系统</div> -->
          <div class="profile-box-header">
            <el-button circle text @click="onCloseModal">
              <el-icon size="20"><Close/></el-icon>
            </el-button>
            <div style="margin-left: 12px; font-weight: bold; font-size: larger;">编辑个人资料</div>
          </div>
          <!-- <el-divider /> -->
          <el-form id="profile-box-form" :model="profileForm" label-width="auto" label-position="left" @keyup.enter.native="onSubmitProfile">
            <el-form-item label="昵称">
              <el-input v-model="profileForm.nickName"/>
            </el-form-item>
            <el-form-item label="简介">
              <el-input v-model="profileForm.introduction"/>
            </el-form-item>
            <el-form-item>
              <el-button style="width: 100%;" type="primary" @click="onSubmitProfile" :loading="isSubmitting">保存</el-button>
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

.profile-box-header {
  display: flex;
  flex-direction: row;
  align-items: center;
}

.profile-box {
  display: flex;
  flex-direction: column;
  padding-left: 1em;
  padding-right: 1em;
  padding-top: 1em;
  padding-bottom: 0.4em;
  background-color: white;
  border-radius: 1em;
  /* box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19); */
}

.input-area {
  margin-top: 12px;
}

.audit-actions-area {
  margin-top: 12px;
  display: flex;
  flex-direction: row;
  justify-content: right;
}

#profile-box-form {
  margin-top: 16px;
}

</style>
