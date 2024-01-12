import { ref, computed } from 'vue'
import { defineStore } from 'pinia'

export const useMainStore = defineStore('main', () => {
  const accessToken = ref('')
  const userName = ref('未登录')
  const nickName = ref('未登录')
  const userId = ref(0)
  const userRights = ref([])

  function setAccessToken(token) {
    accessToken.value = token
  }
  function setUserName(v) {
    userName.value = v
  }
  function setNickName(v) {
    nickName.value = v
  }

  return { accessToken, userName, nickName }
})
