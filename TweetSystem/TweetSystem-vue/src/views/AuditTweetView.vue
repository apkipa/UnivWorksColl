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

const postItem = ref(null);
const remainingPostCount = ref(0);

const onCloseModal = async () => {
  router.back();
};

const loadNewPostToAudit = async () => {
  postItem.value = null;
  const pendingPosts = await client.postAuditListPending();
  if (pendingPosts.length <= 0) {
    return;
  }
  remainingPostCount.value = pendingPosts.length;
  postItem.value = await client.postView(pendingPosts[0].id);
};

const onAuditAccept = async () => {
  await client.postAuditAccept(postItem.value.id);
  await loadNewPostToAudit();
};
const onAuditReject = async () => {
  await client.postAuditReject(postItem.value.id);
  await loadNewPostToAudit();
};

(async function() {
  await loadNewPostToAudit();
})();

</script>

<template>
  <Teleport to="#modal-container">
    <div class="background-overlay">
      <div class="modal-container">
        <div class="post-box">
          <!-- <div id="post-box-header">登录到推文系统</div> -->
          <el-button circle text @click="onCloseModal">
            <el-icon size="20"><Close/></el-icon>
          </el-button>
          <!-- <el-divider /> -->
          <!-- <el-form id="login-box-form" :model="postForm" label-width="auto" label-position="left" @keyup.enter.native="onSubmitLogin">
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
          </el-form> -->
          <div v-if="postItem">
            <div style="margin-top: 8px; margin-bottom: 4px;">这篇推文是否存在问题? (预计剩余: {{ remainingPostCount }} 项)</div>
            <PostItem :post="postItem" :has-actions="false" :can-click-jump="false"/>
            <div class="audit-actions-area">
              <el-button type="success" style="flex: 1;" @click="onAuditAccept">
                <el-icon><Check/></el-icon><div style="margin-left: 4px;">通过</div>
              </el-button>
              <el-button type="danger" style="flex: 1;" @click="onAuditReject">
                <el-icon><Close/></el-icon><div style="margin-left: 4px;">拒绝</div>
              </el-button>
            </div>
          </div>
          <div v-if="!postItem">
            <div>目前没有待审核的推文。可以关闭此弹窗。</div>
          </div>
          <!-- <el-button color="rgb(29, 155, 240)" style="margin-top: 12px; margin-left: auto;">发推</el-button> -->
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
  margin-top: 20vh;
  /* padding-top: 20vh; */
  width: 36vw;
  height: auto;
  background: white;
  border-radius: 15px;
}

.post-box {
  display: flex;
  flex-direction: column;
  padding-left: 1em;
  padding-right: 1em;
  padding-top: 1em;
  padding-bottom: 0.8em;
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

</style>
