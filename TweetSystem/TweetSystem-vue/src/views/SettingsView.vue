<script setup>
import { ref, reactive, VueElement, onMounted, onUnmounted, computed, nextTick } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'
import { useScroll } from '@vueuse/core'
import PostItem from '../components/PostItem.vue'
import { useCookies } from "vue3-cookies";

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()
const { cookies } = useCookies()

const newPassword = ref('')

const onChangePassword = async () => {
  ElMessageBox.prompt('输入新密码:', '修改密码', {
    confirmButtonText: '保存',
    cancelButtonText: '取消修改',
  }).then(async ({ value }) => {
    newPassword.value = value;
    await client.authUpdateInfo(null, null, newPassword.value, null, null, null);
    // Logout and refresh page
    await client.authLogout();
    cookies.remove('satoken');
    router.go();
  });
};

</script>

<template>
  <main style="display: flex; flex-direction: column; box-sizing: border-box;">
    <div class="blur-background page-header-container">
      <div class="page-header">
        <div style="font-size: larger; font-weight: bold; margin: 8px 0;">设置</div>
      </div>
    </div>
    <el-button style="width: 100%; justify-content: left; padding: 24px 12px;" text @click="onChangePassword">
      <el-icon :size="20" style="margin-right: 12px;"><key/></el-icon>更改密码
    </el-button>
  </main>
</template>

<style scoped>
@import '@/assets/styles.css';

.page-header-container {
  position: sticky;
  top: 0;
  padding: 4px 16px;
  z-index: 2;
}

.page-header {
  display: flex;
  flex-direction: column;
}
</style>
